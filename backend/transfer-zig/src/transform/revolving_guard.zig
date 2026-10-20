const std = @import("std");

/// RevolvingGuard - Inspired by referencia previa RevolvingCheckGenerator.
/// Generates junk computations and integrity checks that change execution flow
/// based on dynamic selectors, making static analysis and tracing extremely difficult.

pub const RevolvingGuard = struct {
    pub fn execute(selector: u32, dynamic_val: *u32) void {
        const index = selector % 10;

        // This structure forces the compiler/decompiler to generate complex switch/jump tables
        // which can be combined with Shadow-Mover's remapping for maximum integrity.
        switch (index) {
            0 => {
                dynamic_val.* = (dynamic_val.* +% 0xdead) ^ 0xbeef;
            },
            1 => {
                dynamic_val.* = std.math.rotr(u32, dynamic_val.*, 5) +% 0x1337;
            },
            2 => {
                dynamic_val.* ^= (dynamic_val.* >> 3) | 0xaaaa_bbbb;
            },
            3 => {
                dynamic_val.* = (dynamic_val.* *% 31) -% 1;
            },
            4 => {
                const tmp = dynamic_val.* & 0xff00ff00;
                dynamic_val.* = (tmp >> 8) | (dynamic_val.* << 24);
            },
            5 => {
                dynamic_val.* = dynamic_val.* +% (dynamic_val.* << 1);
            },
            6 => {
                dynamic_val.* = ~dynamic_val.* ^ 0x5555_5555;
            },
            7 => {
                dynamic_val.* = (dynamic_val.* >> 1) +% 0x7fff_ffff;
            },
            8 => {
                dynamic_val.* = std.math.rotl(u32, dynamic_val.*, 13) ^ 0xcafe_babe;
            },
            9 => {
                dynamic_val.* = (dynamic_val.* ^ 0xffff_0000) +% 0x1234;
            },
            else => unreachable,
        }
    }

    /// Generates a "Heartbeat" check - if this isn't called, the artifact can poison itself.
    pub fn verifyIntegrity(actual: u32, expected: u32) bool {
        // Obfuscated comparison
        const diff = actual ^ expected;
        return diff == 0;
    }
};
