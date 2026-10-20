import * as grpc from '@grpc/grpc-js';
import * as protoLoader from '@grpc/proto-loader';
import path from 'path';
import axios from 'axios';
import { chromium } from 'playwright';

// Cargar Protobuf para comunicación con Atlas-Go
const PROTO_PATH = path.resolve(__dirname, '../../proto/atlas.proto');
const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true
});

const atlasProto = grpc.loadPackageDefinition(packageDefinition) as any;
const client = new atlasProto.interlock.atlas.SwarmOrchestrator(
    'localhost:50051',
    grpc.credentials.createInsecure()
);

/**
 * Motor de Inteligencia Web (TypeScript)
 * Especializado en OSINT, Cloud y Automatización de Navegadores.
 */
class IntelEngine {
    constructor() {
        console.log("[Intel-TS] Engine Initialized (NativeAOT-style with Bun)");
    }

    /**
     * Realiza un escaneo OSINT sobre un dominio objetivo
     */
    async performOsint(target: string) {
        console.log(`[Intel-TS] Performing OSINT on: ${target}`);

        // Simulación de búsqueda en Shodan (Ejemplo de facilidad de TS)
        try {
            // En prod: const res = await axios.get(`https://api.shodan.io/shodan/host/${target}?key=API_KEY`);
            const mockFinding = {
                id: `INTEL-${Date.now()}`,
                agent: "vantage-ts",
                title: `Open Ports on ${target}`,
                severity: "MEDIUM",
                description: "Found exposed services: 80, 443, 8080",
                confidence: 0.95,
                metadata: { target, ports: [80, 443, 8080], source: "shodan" }
            };

            this.reportFinding(mockFinding);
        } catch (error) {
            console.error("[Intel-TS] OSINT Error:", error);
        }
    }

    /**
     * Automatización de Cloud (Azure/Entra ID)
     */
    async auditAzureTenant(tenantId: string) {
        console.log(`[Intel-TS] Auditing Azure Tenant: ${tenantId}`);
        // Aquí iría la lógica con @azure/identity
    }

    /**
     * Captura de pantalla y análisis de DOM (Playwright)
     */
    async webRecon(url: string) {
        const browser = await chromium.launch();
        const page = await browser.newPage();
        await page.goto(url);
        const title = await page.title();
        console.log(`[Intel-TS] Web Recon: ${url} -> ${title}`);
        await browser.close();
    }

    private reportFinding(finding: any) {
        const message = {
            id: finding.id,
            from: "intel-ts",
            to: "blackboard",
            subject: "NEW_FINDING",
            artifact: Buffer.from(JSON.stringify(finding)),
            priority: 2, // NORMAL
            timestamp: Date.now()
        };

        client.RouteMessage(message, (err: any, response: any) => {
            if (err) console.error("[Intel-TS] gRPC Error:", err);
            else console.log("[Intel-TS] Finding reported to Atlas-Go.");
        });
    }

    /**
     * Escucha órdenes del Orquestador
     */
    startListening() {
        const call = client.SubscribeMessages({ client_id: "intel-ts", topics: ["intel_tasks"] });

        call.on('data', (msg: any) => {
            console.log(`[Intel-TS] Task received: ${msg.subject}`);
            const task = JSON.parse(msg.artifact.toString());

            if (msg.subject === "OSINT_REQUEST") {
                this.performOsint(task.domain);
            }
        });

        call.on('error', (err: any) => console.error("[Intel-TS] Stream error:", err));
    }
}

const engine = new IntelEngine();
engine.startListening();
