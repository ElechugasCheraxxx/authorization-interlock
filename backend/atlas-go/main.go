package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"syscall"

	"interlock/atlas/internal/blackboard"
	"interlock/atlas/internal/oracle"
	"interlock/atlas/internal/parser"
	"interlock/atlas/pb"
	"google.golang.org/grpc"
	"sync"
	"strings"
)

type atlasServer struct {
	pb.UnimplementedSwarmOrchestratorServer
	msgBus      chan *pb.Message
	subscribers map[string]chan *pb.Message
	subMtx      sync.RWMutex
	blackboard  *blackboard.Blackboard
	oracle      *oracle.GlasswingOracle
}

func newAtlasServer() *atlasServer {
	bb, err := blackboard.NewBlackboard("localhost:6379")
	if err != nil {
		log.Printf("[Atlas] Warning: Redis Blackboard not available: %v", err)
	}

	return &atlasServer{
		msgBus:      make(chan *pb.Message, 1000),
		subscribers: make(map[string]chan *pb.Message),
		blackboard:  bb,
		oracle:      oracle.NewGlasswingOracle(""),
	}
}

func (s *atlasServer) RegisterAgent(ctx context.Context, req *pb.AgentInfo) (*pb.RegistrationResponse, error) {
	log.Printf("[Atlas] Registering Agent: %s (%s/%s)", req.AgentId, req.TargetOs, req.Arch)
	return &pb.RegistrationResponse{
		Success:      true,
		SessionToken: fmt.Sprintf("sess_%s_validated", req.AgentId),
	}, nil
}

func (s *atlasServer) RouteMessage(ctx context.Context, msg *pb.Message) (*pb.RouteResponse, error) {
	log.Printf("[Atlas] Routing Message: [%s] -> [%s] Subject: %s", msg.From, msg.To, msg.Subject)

	// SI EL MENSAJE ES UN OUTPUT DE RAZONAMIENTO DE CIPHER, LO PARSEAMOS
	if msg.Subject == "REASONING_OUTPUT" && s.blackboard != nil {
		go func() {
			subtasks := parser.ParseSubtasks(string(msg.Artifact), msg.From)
			if len(subtasks) > 0 {
				// VALIDACIÓN FORMAL VÍA GLASSWING ORACLE
				taskTitles := make([]string, len(subtasks))
				for i, t := range subtasks {
					taskTitles[i] = t.Title
				}

				valid, err := s.oracle.ValidatePDG(taskTitles)
				if !valid || err != nil {
					log.Printf("🚨 [Atlas] PDG Validation Failed: %v", err)
					// En una operación real, aquí cancelaríamos el plan o pediríamos corrección
					return
				}

				err = s.blackboard.PublishTacticalPlan(subtasks)
				if err != nil {
					log.Printf("[Atlas] Error publishing plan to blackboard: %v", err)
				}
			}
		}()
	}

	// ACTUALIZACIONES DE MISIÓN DESDE AGENTES DE NIVEL 4 (REX, GHOST, WRAITH)
	if msg.Subject == "MISSION_UPDATE" && s.blackboard != nil {
		go func() {
			parts := strings.SplitN(string(msg.Artifact), ":", 2)
			if len(parts) == 2 {
				s.blackboard.PublishMissionUpdate(parts[0], parts[1])
			} else {
				s.blackboard.PublishMissionUpdate(msg.From, string(msg.Artifact))
			}
		}()
	}

	if msg.Subject == "MISSION_RESULT" && s.blackboard != nil {
		go func() {
			// Aquí podríamos validar formalmente el resultado si hay un invariante
			log.Printf("[Atlas] Mission Result from %s: %s", msg.From, string(msg.Artifact))
			s.blackboard.PublishMissionUpdate(msg.From+"_RESULT", string(msg.Artifact))
		}()
	}

	s.subMtx.RLock()
	defer s.subMtx.RUnlock()

	// Broadcast or Targeted Delivery
	delivered := false
	if msg.To == "all" || msg.To == "broadcast" {
		for _, sub := range s.subscribers {
			select {
			case sub <- msg:
				delivered = true
			default:
				log.Printf("[Atlas] Subscriber buffer full, dropping message")
			}
		}
	} else if sub, ok := s.subscribers[msg.To]; ok {
		select {
		case sub <- msg:
			delivered = true
		default:
			log.Printf("[Atlas] Target %s buffer full", msg.To)
		}
	}

	return &pb.RouteResponse{Delivered: delivered}, nil
}

func (s *atlasServer) SubscribeMessages(req *pb.SubscribeRequest, stream pb.SwarmOrchestrator_SubscribeMessagesServer) error {
	log.Printf("[Atlas] Client subscribed: %s to topics %v", req.ClientId, req.Topics)

	msgChan := make(chan *pb.Message, 100)

	s.subMtx.Lock()
	s.subscribers[req.ClientId] = msgChan
	s.subMtx.Unlock()

	defer func() {
		s.subMtx.Lock()
		delete(s.subscribers, req.ClientId)
		s.subMtx.Unlock()
		close(msgChan)
	}()

	for {
		select {
		case msg := <-msgChan:
			if err := stream.Send(msg); err != nil {
				return err
			}
		case <-stream.Context().Done():
			return stream.Context().Err()
		}
	}
}

func (s *atlasServer) TunnelTelemetria(stream pb.SwarmOrchestrator_TunnelTelemetriaServer) error {
	log.Println("[Atlas] New Telemetry Tunnel opened")
	for {
		data, err := stream.Recv()
		if err != nil {
			log.Printf("[Atlas] Tunnel closed: %v", err)
			return err
		}
		log.Printf("[Atlas] Received Telemetry from %s: %d bytes", data.AgentId, len(data.Artifact))

		// Responder con un comando de control (ejemplo: KeepAlive)
		err = stream.Send(&pb.ControlCommand{
			CommandId: "cmd_ack",
			Action:    "CONTINUE",
		})
		if err != nil {
			return err
		}
	}
}

func main() {
	// Puerto TCP para agentes externos (como C#)
	tcpPort := "127.0.0.1:50051"
	lisTCP, err := net.Listen("tcp", tcpPort)
	if err != nil {
		log.Fatalf("failed to listen TCP: %v", err)
	}

	// Socket Unix para C++ Core
	socketPath := "/tmp/interlock_atlas.sock"
	if _, err := os.Stat(socketPath); err == nil {
		if err := os.Remove(socketPath); err != nil {
			log.Fatalf("failed to remove stale socket: %v", err)
		}
	}
	lisUnix, err := net.Listen("unix", socketPath)
	if err != nil {
		log.Fatalf("failed to listen Unix: %v", err)
	}

	s := grpc.NewServer()
	pb.RegisterSwarmOrchestratorServer(s, newAtlasServer())

	log.Printf("[Atlas] Swarm Orchestrator starting on %s and %s", socketPath, tcpPort)

	go func() {
		if err := s.Serve(lisTCP); err != nil {
			log.Fatalf("failed to serve TCP: %v", err)
		}
	}()

	// Manejo de señales para apagado limpio
	go func() {
		sigCh := make(chan os.Signal, 1)
		signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
		<-sigCh
		log.Println("[Atlas] Shutting down...")
		s.GracefulStop()
	}()

	if err := s.Serve(lisUnix); err != nil {
		log.Fatalf("failed to serve Unix: %v", err)
	}
}
