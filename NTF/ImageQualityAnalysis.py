"""
图像质量分析脚本
用于比较参考图像和多张渲染图的质量差异

主要指标：
1. PSNR (Peak Signal-to-Noise Ratio) - 峰值信噪比
2. SSIM (Structural Similarity Index) - 结构相似性指数
3. MS-SSIM (Multi-Scale SSIM) - 多尺度结构相似性
4. LPIPS (Learned Perceptual Image Patch Similarity) - 感知相似性
5. MSE (Mean Squared Error) - 均方误差
6. MAE (Mean Absolute Error) - 平均绝对误差

输出分析图：
1. 并排对比图
2. 差异热力图
3. SSIM 局部分布图
4. 指标柱状图对比
5. 误差直方图
"""

import numpy as np
import cv2
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import matplotlib.patches as mpatches
from skimage.metrics import structural_similarity as ssim
from skimage.metrics import peak_signal_noise_ratio as psnr
from skimage.metrics import mean_squared_error as mse
import warnings

warnings.filterwarnings('ignore')

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial Unicode MS']
plt.rcParams['axes.unicode_minus'] = False


class ImageQualityAnalyzer:
    """图像质量分析器"""

    def __init__(self, reference_path: str, render_paths: list, labels: list = None):
        """
        初始化分析器

        Args:
            reference_path: 参考图像路径
            render_paths: 渲染图像路径列表
            labels: 各渲染图的标签名称
        """
        self.reference_path = reference_path
        self.render_paths = render_paths
        self.labels = labels if labels else [f"Render {i + 1}" for i in range(len(render_paths))]

        # 加载图像
        self.reference = self._load_image(reference_path)
        self.renders = [self._load_image(p) for p in render_paths]

        # 存储计算结果
        self.metrics = {}
        self.ssim_maps = []
        self.diff_maps = []

    def _load_image(self, path: str) -> np.ndarray:
        """加载图像并转换为RGB格式"""
        img = cv2.imread(str(path))
        if img is None:
            raise FileNotFoundError(f"无法加载图像: {path}")
        return cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    def _to_grayscale(self, img: np.ndarray) -> np.ndarray:
        """转换为灰度图"""
        return cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)

    def compute_psnr(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算 PSNR"""
        return psnr(img1, img2, data_range=255)

    def compute_ssim(self, img1: np.ndarray, img2: np.ndarray, return_map: bool = False):
        """计算 SSIM，可选返回局部 SSIM 图"""
        gray1 = self._to_grayscale(img1)
        gray2 = self._to_grayscale(img2)

        if return_map:
            score, ssim_map = ssim(gray1, gray2, full=True, data_range=255)
            return score, ssim_map
        else:
            return ssim(gray1, gray2, data_range=255)

    def compute_ms_ssim(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算多尺度 SSIM"""
        try:
            from pytorch_msssim import ms_ssim
            import torch

            # 转换为 tensor
            t1 = torch.from_numpy(img1).permute(2, 0, 1).unsqueeze(0).float() / 255.0
            t2 = torch.from_numpy(img2).permute(2, 0, 1).unsqueeze(0).float() / 255.0

            return ms_ssim(t1, t2, data_range=1.0).item()
        except ImportError:
            # 如果没有安装 pytorch_msssim，使用简化版本
            return self._compute_ms_ssim_simple(img1, img2)

    def _compute_ms_ssim_simple(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """简化版多尺度 SSIM"""
        weights = [0.0448, 0.2856, 0.3001, 0.2363, 0.1333]
        levels = len(weights)

        mssim = []
        for i in range(levels):
            ssim_val = self.compute_ssim(img1, img2)
            mssim.append(ssim_val)

            # 下采样
            img1 = cv2.resize(img1, (img1.shape[1] // 2, img1.shape[0] // 2))
            img2 = cv2.resize(img2, (img2.shape[1] // 2, img2.shape[0] // 2))

            if img1.shape[0] < 16 or img1.shape[1] < 16:
                break

        # 加权平均
        weights = weights[:len(mssim)]
        weights = [w / sum(weights) for w in weights]
        return sum(w * s for w, s in zip(weights, mssim))

    def compute_mse(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算 MSE"""
        return mse(img1, img2)

    def compute_mae(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算 MAE"""
        return np.mean(np.abs(img1.astype(float) - img2.astype(float)))

    def compute_rmse(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算 RMSE"""
        return np.sqrt(self.compute_mse(img1, img2))

    def compute_lpips(self, img1: np.ndarray, img2: np.ndarray) -> float:
        """计算 LPIPS（需要安装 lpips 库）"""
        try:
            import lpips
            import torch

            loss_fn = lpips.LPIPS(net='alex', verbose=False)

            # 转换为 tensor，范围 [-1, 1]
            t1 = torch.from_numpy(img1).permute(2, 0, 1).unsqueeze(0).float() / 127.5 - 1
            t2 = torch.from_numpy(img2).permute(2, 0, 1).unsqueeze(0).float() / 127.5 - 1

            with torch.no_grad():
                return loss_fn(t1, t2).item()
        except ImportError:
            return None

    def compute_all_metrics(self):
        """计算所有指标"""
        print("正在计算图像质量指标...")

        for i, (render, label) in enumerate(zip(self.renders, self.labels)):
            print(f"  处理: {label}")

            # 基础指标
            psnr_val = self.compute_psnr(self.reference, render)
            ssim_val, ssim_map = self.compute_ssim(self.reference, render, return_map=True)
            ms_ssim_val = self.compute_ms_ssim(self.reference, render)
            mse_val = self.compute_mse(self.reference, render)
            mae_val = self.compute_mae(self.reference, render)
            rmse_val = self.compute_rmse(self.reference, render)

            # LPIPS (可选)
            lpips_val = self.compute_lpips(self.reference, render)

            # 差异图
            diff_map = np.abs(self.reference.astype(float) - render.astype(float))
            diff_map = np.mean(diff_map, axis=2)  # 转为灰度

            self.metrics[label] = {
                'PSNR (dB)': psnr_val,
                'SSIM': ssim_val,
                'MS-SSIM': ms_ssim_val,
                'MSE': mse_val,
                'MAE': mae_val,
                'RMSE': rmse_val,
                'LPIPS': lpips_val
            }

            self.ssim_maps.append(ssim_map)
            self.diff_maps.append(diff_map)

        return self.metrics

    def print_metrics_table(self):
        """打印指标表格"""
        print("\n" + "=" * 80)
        print("图像质量指标对比")
        print("=" * 80)

        # 表头
        header = f"{'指标':<15}"
        for label in self.labels:
            header += f"{label:<20}"
        print(header)
        print("-" * 80)

        # 指标行
        metric_names = ['PSNR (dB)', 'SSIM', 'MS-SSIM', 'MSE', 'MAE', 'RMSE', 'LPIPS']
        for metric in metric_names:
            row = f"{metric:<15}"
            for label in self.labels:
                val = self.metrics[label][metric]
                if val is not None:
                    if metric in ['PSNR (dB)']:
                        row += f"{val:<20.4f}"
                    elif metric in ['SSIM', 'MS-SSIM']:
                        row += f"{val:<20.6f}"
                    elif metric == 'LPIPS':
                        row += f"{val:<20.6f}"
                    else:
                        row += f"{val:<20.4f}"
                else:
                    row += f"{'N/A':<20}"
            print(row)

        print("=" * 80)

        # 最佳指标说明
        print("\n指标说明:")
        print("  PSNR: 越高越好 (>30dB 通常认为质量较好)")
        print("  SSIM/MS-SSIM: 越接近1越好")
        print("  MSE/MAE/RMSE: 越低越好")
        print("  LPIPS: 越低越好 (感知相似度)")

    def plot_comparison(self, output_dir: str = "analysis_output"):
        """生成所有分析图"""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)

        print(f"\n正在生成分析图到: {output_path}")

        # 1. 并排对比图
        self._plot_side_by_side(output_path / "1_comparison.png")

        # 2. 差异热力图
        self._plot_difference_heatmaps(output_path / "2_difference_heatmaps.png")

        # 3. SSIM 分布图
        self._plot_ssim_maps(output_path / "3_ssim_maps.png")

        # 4. 指标柱状图
        self._plot_metrics_bar(output_path / "4_metrics_comparison.png")

        # 5. 误差直方图
        self._plot_error_histograms(output_path / "5_error_histograms.png")

        # 6. 综合分析图
        self._plot_comprehensive_analysis(output_path / "6_comprehensive_analysis.png")

        # 7. 局部放大对比
        self._plot_zoomed_comparison(output_path / "7_zoomed_comparison.png")

        # 8. 雷达图
        self._plot_radar_chart(output_path / "8_radar_chart.png")

        print("分析图生成完成!")

    def _plot_side_by_side(self, save_path: str):
        """并排对比图"""
        n = len(self.renders) + 1
        fig, axes = plt.subplots(1, n, figsize=(5 * n, 5))

        # 参考图
        axes[0].imshow(self.reference)
        axes[0].set_title("Reference (Ground Truth)", fontsize=12, fontweight='bold')
        axes[0].axis('off')

        # 渲染图
        for i, (render, label) in enumerate(zip(self.renders, self.labels)):
            axes[i + 1].imshow(render)
            psnr_val = self.metrics[label]['PSNR (dB)']
            ssim_val = self.metrics[label]['SSIM']
            axes[i + 1].set_title(f"{label}\nPSNR: {psnr_val:.2f}dB, SSIM: {ssim_val:.4f}", fontsize=10)
            axes[i + 1].axis('off')

        plt.suptitle("Image Comparison", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_difference_heatmaps(self, save_path: str):
        """差异热力图"""
        n = len(self.renders)
        fig, axes = plt.subplots(2, n, figsize=(5 * n, 10))

        if n == 1:
            axes = axes.reshape(2, 1)

        for i, (render, diff_map, label) in enumerate(zip(self.renders, self.diff_maps, self.labels)):
            # 原图
            axes[0, i].imshow(render)
            axes[0, i].set_title(label, fontsize=12)
            axes[0, i].axis('off')

            # 差异图
            im = axes[1, i].imshow(diff_map, cmap='hot', vmin=0, vmax=50)
            axes[1, i].set_title(f"Difference Map\nMAE: {self.metrics[label]['MAE']:.2f}", fontsize=10)
            axes[1, i].axis('off')
            plt.colorbar(im, ax=axes[1, i], fraction=0.046, pad=0.04)

        plt.suptitle("Difference Heatmaps (Reference vs Renders)", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_ssim_maps(self, save_path: str):
        """SSIM 局部分布图"""
        n = len(self.renders)
        fig, axes = plt.subplots(2, n, figsize=(5 * n, 10))

        if n == 1:
            axes = axes.reshape(2, 1)

        for i, (render, ssim_map, label) in enumerate(zip(self.renders, self.ssim_maps, self.labels)):
            # 原图
            axes[0, i].imshow(render)
            axes[0, i].set_title(label, fontsize=12)
            axes[0, i].axis('off')

            # SSIM 图
            im = axes[1, i].imshow(ssim_map, cmap='RdYlGn', vmin=0.8, vmax=1.0)
            axes[1, i].set_title(f"Local SSIM\nMean: {self.metrics[label]['SSIM']:.4f}", fontsize=10)
            axes[1, i].axis('off')
            plt.colorbar(im, ax=axes[1, i], fraction=0.046, pad=0.04)

        plt.suptitle("SSIM Distribution Maps", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_metrics_bar(self, save_path: str):
        """指标柱状图对比"""
        fig, axes = plt.subplots(2, 3, figsize=(15, 10))

        labels = self.labels
        x = np.arange(len(labels))
        width = 0.6

        colors = plt.cm.Set2(np.linspace(0, 1, len(labels)))

        # PSNR
        values = [self.metrics[l]['PSNR (dB)'] for l in labels]
        bars = axes[0, 0].bar(x, values, width, color=colors)
        axes[0, 0].set_ylabel('PSNR (dB)')
        axes[0, 0].set_title('PSNR Comparison (Higher is Better)')
        axes[0, 0].set_xticks(x)
        axes[0, 0].set_xticklabels(labels, rotation=15, ha='right')
        for bar, val in zip(bars, values):
            axes[0, 0].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.5,
                            f'{val:.2f}', ha='center', va='bottom', fontsize=9)

        # SSIM
        values = [self.metrics[l]['SSIM'] for l in labels]
        bars = axes[0, 1].bar(x, values, width, color=colors)
        axes[0, 1].set_ylabel('SSIM')
        axes[0, 1].set_title('SSIM Comparison (Higher is Better)')
        axes[0, 1].set_xticks(x)
        axes[0, 1].set_xticklabels(labels, rotation=15, ha='right')
        axes[0, 1].set_ylim([min(values) - 0.01, 1.0])
        for bar, val in zip(bars, values):
            axes[0, 1].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.002,
                            f'{val:.4f}', ha='center', va='bottom', fontsize=9)

        # MS-SSIM
        values = [self.metrics[l]['MS-SSIM'] for l in labels]
        bars = axes[0, 2].bar(x, values, width, color=colors)
        axes[0, 2].set_ylabel('MS-SSIM')
        axes[0, 2].set_title('MS-SSIM Comparison (Higher is Better)')
        axes[0, 2].set_xticks(x)
        axes[0, 2].set_xticklabels(labels, rotation=15, ha='right')
        axes[0, 2].set_ylim([min(values) - 0.01, 1.0])
        for bar, val in zip(bars, values):
            axes[0, 2].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.002,
                            f'{val:.4f}', ha='center', va='bottom', fontsize=9)

        # MSE
        values = [self.metrics[l]['MSE'] for l in labels]
        bars = axes[1, 0].bar(x, values, width, color=colors)
        axes[1, 0].set_ylabel('MSE')
        axes[1, 0].set_title('MSE Comparison (Lower is Better)')
        axes[1, 0].set_xticks(x)
        axes[1, 0].set_xticklabels(labels, rotation=15, ha='right')
        for bar, val in zip(bars, values):
            axes[1, 0].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max(values) * 0.02,
                            f'{val:.2f}', ha='center', va='bottom', fontsize=9)

        # RMSE
        values = [self.metrics[l]['RMSE'] for l in labels]
        bars = axes[1, 1].bar(x, values, width, color=colors)
        axes[1, 1].set_ylabel('RMSE')
        axes[1, 1].set_title('RMSE Comparison (Lower is Better)')
        axes[1, 1].set_xticks(x)
        axes[1, 1].set_xticklabels(labels, rotation=15, ha='right')
        for bar, val in zip(bars, values):
            axes[1, 1].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max(values) * 0.02,
                            f'{val:.2f}', ha='center', va='bottom', fontsize=9)

        # LPIPS (if available)
        values = [self.metrics[l]['LPIPS'] for l in labels]
        if all(v is not None for v in values):
            bars = axes[1, 2].bar(x, values, width, color=colors)
            axes[1, 2].set_ylabel('LPIPS')
            axes[1, 2].set_title('LPIPS Comparison (Lower is Better)')
            axes[1, 2].set_xticks(x)
            axes[1, 2].set_xticklabels(labels, rotation=15, ha='right')
            for bar, val in zip(bars, values):
                axes[1, 2].text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max(values) * 0.02,
                                f'{val:.4f}', ha='center', va='bottom', fontsize=9)
        else:
            axes[1, 2].text(0.5, 0.5, 'LPIPS not available\n(Install lpips package)',
                            ha='center', va='center', transform=axes[1, 2].transAxes, fontsize=12)
            axes[1, 2].set_title('LPIPS Comparison')

        plt.suptitle("Quality Metrics Comparison", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_error_histograms(self, save_path: str):
        """误差直方图"""
        n = len(self.renders)
        fig, axes = plt.subplots(1, n, figsize=(5 * n, 4))

        if n == 1:
            axes = [axes]

        colors = plt.cm.Set2(np.linspace(0, 1, n))

        for i, (diff_map, label, color) in enumerate(zip(self.diff_maps, self.labels, colors)):
            # 展平差异图
            errors = diff_map.flatten()

            axes[i].hist(errors, bins=50, color=color, alpha=0.7, edgecolor='black')
            axes[i].set_xlabel('Pixel Error')
            axes[i].set_ylabel('Frequency')
            axes[i].set_title(f'{label}\nMean: {np.mean(errors):.2f}, Std: {np.std(errors):.2f}')
            axes[i].axvline(np.mean(errors), color='red', linestyle='--', label=f'Mean: {np.mean(errors):.2f}')
            axes[i].legend()

        plt.suptitle("Error Distribution Histograms", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_comprehensive_analysis(self, save_path: str):
        """综合分析图"""
        fig = plt.figure(figsize=(20, 12))
        gs = GridSpec(3, 4, figure=fig, hspace=0.3, wspace=0.3)

        n = len(self.renders)

        # 第一行：参考图和渲染图
        ax_ref = fig.add_subplot(gs[0, 0])
        ax_ref.imshow(self.reference)
        ax_ref.set_title("Reference", fontsize=10, fontweight='bold')
        ax_ref.axis('off')

        for i in range(min(n, 3)):
            ax = fig.add_subplot(gs[0, i + 1])
            ax.imshow(self.renders[i])
            ax.set_title(f"{self.labels[i]}", fontsize=10)
            ax.axis('off')

        # 第二行：差异图
        for i in range(min(n, 4)):
            ax = fig.add_subplot(gs[1, i])
            im = ax.imshow(self.diff_maps[i], cmap='hot', vmin=0, vmax=30)
            ax.set_title(f"Diff: {self.labels[i]}", fontsize=9)
            ax.axis('off')

        # 第三行：指标对比
        ax_metrics = fig.add_subplot(gs[2, :2])
        metrics_to_plot = ['PSNR (dB)', 'SSIM', 'MS-SSIM']
        x = np.arange(len(self.labels))
        width = 0.25

        for j, metric in enumerate(metrics_to_plot):
            values = [self.metrics[l][metric] for l in self.labels]
            if metric == 'PSNR (dB)':
                values = [v / 50 for v in values]  # 归一化
            ax_metrics.bar(x + j * width, values, width, label=metric)

        ax_metrics.set_xticks(x + width)
        ax_metrics.set_xticklabels(self.labels, rotation=15)
        ax_metrics.legend()
        ax_metrics.set_title("Normalized Metrics Comparison", fontsize=10)
        ax_metrics.set_ylabel("Normalized Value")

        # SSIM 分布
        ax_ssim = fig.add_subplot(gs[2, 2:])
        for i, (ssim_map, label) in enumerate(zip(self.ssim_maps, self.labels)):
            ssim_values = ssim_map.flatten()
            ax_ssim.hist(ssim_values, bins=50, alpha=0.5, label=label)
        ax_ssim.set_xlabel('Local SSIM')
        ax_ssim.set_ylabel('Frequency')
        ax_ssim.set_title("Local SSIM Distribution", fontsize=10)
        ax_ssim.legend()

        plt.suptitle("Comprehensive Image Quality Analysis", fontsize=14, fontweight='bold')
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_zoomed_comparison(self, save_path: str, zoom_region: tuple = None):
        """局部放大对比图"""
        h, w = self.reference.shape[:2]

        # 默认取中心区域
        if zoom_region is None:
            size = min(h, w) // 4
            y1, x1 = h // 2 - size // 2, w // 2 - size // 2
            y2, x2 = y1 + size, x1 + size
        else:
            x1, y1, x2, y2 = zoom_region

        n = len(self.renders) + 1
        fig, axes = plt.subplots(2, n, figsize=(4 * n, 8))

        # 第一行：全图
        axes[0, 0].imshow(self.reference)
        rect = plt.Rectangle((x1, y1), x2 - x1, y2 - y1, fill=False, edgecolor='red', linewidth=2)
        axes[0, 0].add_patch(rect)
        axes[0, 0].set_title("Reference", fontsize=10)
        axes[0, 0].axis('off')

        for i, (render, label) in enumerate(zip(self.renders, self.labels)):
            axes[0, i + 1].imshow(render)
            rect = plt.Rectangle((x1, y1), x2 - x1, y2 - y1, fill=False, edgecolor='red', linewidth=2)
            axes[0, i + 1].add_patch(rect)
            axes[0, i + 1].set_title(label, fontsize=10)
            axes[0, i + 1].axis('off')

        # 第二行：放大区域
        axes[1, 0].imshow(self.reference[y1:y2, x1:x2])
        axes[1, 0].set_title("Reference (Zoomed)", fontsize=10)
        axes[1, 0].axis('off')

        for i, (render, label) in enumerate(zip(self.renders, self.labels)):
            axes[1, i + 1].imshow(render[y1:y2, x1:x2])
            psnr_val = psnr(self.reference[y1:y2, x1:x2], render[y1:y2, x1:x2], data_range=255)
            axes[1, i + 1].set_title(f"{label} (Zoomed)\nLocal PSNR: {psnr_val:.2f}dB", fontsize=9)
            axes[1, i + 1].axis('off')

        plt.suptitle("Zoomed Region Comparison", fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def _plot_radar_chart(self, save_path: str):
        """雷达图对比"""
        # 选择用于雷达图的指标
        metrics_for_radar = ['PSNR (dB)', 'SSIM', 'MS-SSIM']

        # 检查 LPIPS 是否可用
        lpips_available = all(self.metrics[l]['LPIPS'] is not None for l in self.labels)
        if lpips_available:
            metrics_for_radar.append('LPIPS (inv)')  # 反转 LPIPS

        n_metrics = len(metrics_for_radar)
        angles = np.linspace(0, 2 * np.pi, n_metrics, endpoint=False).tolist()
        angles += angles[:1]  # 闭合

        fig, ax = plt.subplots(figsize=(8, 8), subplot_kw=dict(polar=True))

        colors = plt.cm.Set2(np.linspace(0, 1, len(self.labels)))

        for i, (label, color) in enumerate(zip(self.labels, colors)):
            values = []
            for metric in metrics_for_radar:
                if metric == 'LPIPS (inv)':
                    # 反转 LPIPS (1 - LPIPS)，使其也是越高越好
                    val = 1 - self.metrics[label]['LPIPS']
                elif metric == 'PSNR (dB)':
                    # 归一化 PSNR (假设范围 20-50)
                    val = (self.metrics[label][metric] - 20) / 30
                else:
                    val = self.metrics[label][metric]
                values.append(val)

            values += values[:1]  # 闭合
            ax.plot(angles, values, 'o-', linewidth=2, label=label, color=color)
            ax.fill(angles, values, alpha=0.25, color=color)

        ax.set_xticks(angles[:-1])
        ax.set_xticklabels(metrics_for_radar)
        ax.set_ylim(0, 1)
        ax.legend(loc='upper right', bbox_to_anchor=(1.3, 1.0))

        plt.title("Quality Metrics Radar Chart\n(All metrics normalized, higher is better)", fontsize=12,
                  fontweight='bold')
        plt.tight_layout()
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        plt.close()
        print(f"  保存: {save_path}")

    def export_to_csv(self, output_path: str = "analysis_output/metrics.csv"):
        """导出指标到 CSV"""
        import csv

        Path(output_path).parent.mkdir(exist_ok=True)

        with open(output_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)

            # 表头
            header = ['Metric'] + self.labels
            writer.writerow(header)

            # 数据行
            metric_names = ['PSNR (dB)', 'SSIM', 'MS-SSIM', 'MSE', 'MAE', 'RMSE', 'LPIPS']
            for metric in metric_names:
                row = [metric]
                for label in self.labels:
                    val = self.metrics[label][metric]
                    row.append(val if val is not None else 'N/A')
                writer.writerow(row)

        print(f"  指标已导出到: {output_path}")


def main():
    """主函数 - 示例用法"""

    # ============================================
    # 请修改以下路径为你的实际图像路径
    # ============================================

    # 参考图像（Ground Truth）
    reference_path = "assets/Images/Bayon Lion_Reference.png"

    # 渲染图像列表
    render_paths = [
        "assets/Images/Bayon Lion_DistanceBased.png",
        "assets/Images/Bayon Lion_ScreenSpace.png",
        "assets/Images/Bayon Lion_NTF.png",
    ]

    # 对应的标签名称（用于图表显示）
    labels = [
        "Distance-Based",
        "Screen-Space",
        "NTF (Ours)",
    ]

    # ============================================
    # 运行分析
    # ============================================

    print("=" * 60)
    print("图像质量分析工具")
    print("=" * 60)

    try:
        # 创建分析器
        analyzer = ImageQualityAnalyzer(reference_path, render_paths, labels)

        # 计算所有指标
        analyzer.compute_all_metrics()

        # 打印指标表格
        analyzer.print_metrics_table()

        # 生成分析图
        analyzer.plot_comparison(output_dir="analysis_output")

        # 导出 CSV
        analyzer.export_to_csv()

        print("\n分析完成！")

    except FileNotFoundError as e:
        print(f"\n错误: {e}")
        print("\n请确保以下图像文件存在:")
        print(f"  参考图像: {reference_path}")
        for p in render_paths:
            print(f"  渲染图像: {p}")
        print("\n你可以修改 main() 函数中的路径来指向你的图像文件。")


if __name__ == "__main__":
    main()
