# string_sanitizer.rb - Hostile input sanitizer
# Randomizes or strips strings in binary payloads to thwart RTTI analysis and strings-based detection.

require 'fileutils'

module Interlock
  class StringSanitizer
    def self.sanitize(file_path, blacklist_patterns)
      puts "[StringSanitizer] Dissecting binary: #{File.basename(file_path)}"

      return unless File.exist?(file_path)

      # Read binary data
      binary_data = File.binread(file_path)
      modified = false

      blacklist_patterns.each do |pattern|
        # Find occurrences of the pattern in the binary
        # We look for null-terminated strings or specific byte sequences
        regex = Regexp.new(Regexp.escape(pattern))

        while (match = binary_data.match(regex))
          offset = match.begin(0)
          length = match[0].length

          puts "[StringSanitizer] Found blacklisted string '#{pattern}' at 0x#{offset.to_s(16)}. Randomizing..."

          # Replace with random bytes, keeping null terminator if it exists
          random_bytes = Random.new.bytes(length)
          binary_data[offset, length] = random_bytes
          modified = true
        end
      end

      if modified
        sanitized_path = file_path + ".sanitized"
        File.binwrite(sanitized_path, binary_data)
        puts "[StringSanitizer] Sanitized binary written to: #{sanitized_path}"
        sanitized_path
      else
        puts "[StringSanitizer] No blacklisted strings found."
        file_path
      end
    end
  end
end
