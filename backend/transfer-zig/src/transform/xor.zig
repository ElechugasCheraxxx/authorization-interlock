const std = @import("std");

/// Simple XOR obfuscation performed at COMPILE TIME.
pub fn xor_string(comptime input: []const u8, comptime key: u8) [input.len]u8 {
    var output: [input.len]u8 = undefined;
    for (input, 0..) |char, i| {
        output[i] = char ^ key;
    }
    return output;
}

/// Runtime XOR de-obfuscation.
pub fn xor_buffer(output: []u8, input: []const u8, key: u8) void {
    for (input, 0..) |char, i| {
        output[i] = char ^ key;
    }
}
