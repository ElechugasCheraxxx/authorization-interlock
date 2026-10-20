const std = @import("std");

/// Scrambler - Inspired by referencia previa NativeScrambler
/// Provides compile-time and run-time instruction/command shuffling.

pub fn Scrambler(comptime T: type) type {
    return struct {
        commands: []T,
        indices: []usize,
        allocator: std.mem.Allocator,

        pub fn init(allocator: std.mem.Allocator, commands: []T) !@This() {
            var indices = try allocator.alloc(usize, commands.len);
            for (indices, 0..) |*idx, i| {
                idx.* = i;
            }
            return .{
                .commands = commands,
                .indices = indices,
                .allocator = allocator,
            };
        }

        pub fn deinit(self: *@This()) void {
            self.allocator.free(self.indices);
        }

        /// Shuffles the execution order using a PRNG
        pub fn shuffle(self: *@This(), seed: u64) void {
            var prng = std.rand.DefaultPrng.init(seed);
            const rand = prng.random();

            // Fisher-Yates shuffle
            var i: usize = self.indices.len - 1;
            while (i > 0) : (i -= 1) {
                const j = rand.uintLessThan(usize, i + 1);
                std.mem.swap(usize, &self.indices[i], &self.indices[j]);
            }
        }

        /// Executes commands in the scrambled order
        pub fn execute(self: *@This(), context: anytype, func: anytype) void {
            for (self.indices) |idx| {
                func(context, self.commands[idx]);
            }
        }
    };
}
