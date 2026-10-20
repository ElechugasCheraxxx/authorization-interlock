const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // En Zig 0.16.0, addStaticLibrary y addSharedLibrary han sido unificados en addLibrary.
    // Se requiere crear un módulo raíz explícitamente y pasarlo a addLibrary.
    const lib = b.addLibrary(.{
        .name = "interlock_payloads",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/root.zig"),
            .target = target,
            .optimize = optimize,
        }),
        .linkage = .static,
    });

    // Permitir que C++ vea los headers generados o use la lib estática
    b.installArtifact(lib);
}
