struct Ray {
    float3 Origin;
    float3 Direction;
};

struct RayGenData {};

struct ClosestHitData {};

struct MissData {};

struct LaunchParams {
    OptixTraversableHandle Handle;
    Ray* Rays;
    uint32_t* VisBuffer;
    uint32_t Width;
    uint32_t Height;
};
