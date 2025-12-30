export module iGe.CommonFunctions;
import iGe.Types;
import iGe.Diagnostics;

namespace iGe
{

export string ReadFile(const std::filesystem::path& filepath) {
    // Check if file exists
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        Internal::LogError("File does not exist: '{0}' (error: {1})", filepath.string(), ec.message());
        return {};
    }

    // Get the file size
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        Internal::LogError("Could not open file: '{0}'", filepath.string());
        return {};
    }

    auto fileSize = file.tellg();
    file.seekg(0);

    // Read file content
    string content;
    content.resize(fileSize);

    file.read(content.data(), fileSize);
    if (!file) {
        Internal::LogError("Could not read file: '{0}'", filepath.string());
        return {};
    }

    return content;
}

} // namespace iGe
