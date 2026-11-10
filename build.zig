const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const raylib_dep = b.dependency("raylib_zig", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "gorgonzola",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.root_module.addImport("raylib", raylib_dep.module("raylib"));
    exe.root_module.linkLibrary(raylib_dep.artifact("raylib"));

    b.installArtifact(exe);

    const run = b.addRunArtifact(exe);
    b.step("run", "Run Gorgonzola").dependOn(&run.step);
}
