require 'json'
require 'socket'

# Interlock Ruby Tool Dispatcher DSL
# Provee una interfaz flexible para orquestar herramientas externas.

class ToolOrchestrator
  def initialize
    @tools = {}
  end

  def register_tool(name, &block)
    @tools[name] = block
  end

  def execute(tool_name, params)
    if @tools.key?(tool_name)
      @tools[tool_name].call(params)
    else
      { error: "Tool #{tool_name} not found in Ruby DSL" }.to_json
    end
  end
end

orchestrator = ToolOrchestrator.new

# Ejemplo de DSL para Nmap
orchestrator.register_tool("nmap_scan") do |params|
  target = params["target"]
  puts "[Ruby-DSL] Running Nmap scan on #{target}..."
  # Aquí se ejecutaría la lógica real de orquestación
  { status: "success", result: "Scan completed for #{target}" }.to_json
end

# Comunicación vía IPC (Unix Domain Socket)
socket_path = "/tmp/interlock_ipc_12" # RUBY_DISPATCHER = 12

begin
  File.unlink(socket_path) if File.exist?(socket_path)
  server = UNIXServer.new(socket_path)
  puts "[Ruby] Dispatcher listening on #{socket_path}"

  loop do
    client = server.accept
    begin
      data = client.read(65536) # IPCMessage::MAX_PAYLOAD
      # El protocolo IPCMessage tiene un header de 32 bytes antes del artifact
      # Para simplificar este prototipo, asumimos que el artifact es JSON directo
      # En una integración final, usaríamos una gema para parsear el struct de C++

      request = JSON.parse(data[32..-1].strip)
      result = orchestrator.execute(request["tool"], request["params"])

      client.write(result)
    rescue => e
      puts "[Ruby] Error: #{e.message}"
    ensure
      client.close
    end
  end
ensure
  File.unlink(socket_path) if File.exist?(socket_path)
end
