const rl = @import("raylib");
const terrain = @import("terrain.zig");
const stuff = @import("stuff.zig");
const std = @import("std");

pub fn main() void {
    rl.initWindow(800, 600, "Gorgonzola");
    defer rl.closeWindow();

    rl.setTargetFPS(60);

    var camera = terrain.Camera{
        .x = 100,
        .y = 100,
    };

    terrain.init(std.heap.page_allocator);
    defer terrain.deinit();

    stuff.init(std.heap.page_allocator);
    defer stuff.deinit();

    while (!rl.windowShouldClose()) {
        const dt = rl.getFrameTime();

        if (rl.isKeyDown(.w)) camera.y -= 4;
        if (rl.isKeyDown(.s)) camera.y += 4;
        if (rl.isKeyDown(.a)) camera.x -= 4;
        if (rl.isKeyDown(.d)) camera.x += 4;

        if (rl.isMouseButtonPressed(.left)) {
            const mouse = rl.getMousePosition();

            stuff.spawnBomb(
                mouse.x + @as(f32, @floatFromInt(camera.x)),
                mouse.y + @as(f32, @floatFromInt(camera.y)),
            );
        }
        stuff.update(dt);

        rl.beginDrawing();
        rl.clearBackground(.black);

        terrain.drawterrain(camera);
        stuff.draw(camera);

        rl.endDrawing();
    }
}
