import asyncio
import logging
import time
import sys
import os
import random
import statistics
from pathlib import Path

# Add project root to path
sys.path.append(str(Path(__file__).resolve().parent.parent / "backend"))

from model_layer.llama_cpp_adapter import LlamaCppAdapter
from model_layer.model_registry import ModelConfig

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
)
_log = logging.getLogger("LLAMA-STRESS-TEST")

class LlamaStressTester:
    def __init__(self, config: ModelConfig, concurrency: int = 5, total_requests: int = 50):
        self.config = config
        self.adapter = LlamaCppAdapter(config)
        self.concurrency = concurrency
        self.total_requests = total_requests
        self.results = []
        self.errors = 0
        self.latencies = []

    async def run_worker(self, worker_id: int, queue: asyncio.Queue):
        while not queue.empty():
            req_id = await queue.get()
            _log.info(f"Worker {worker_id} starting request {req_id}")

            start_time = time.perf_counter()
            try:
                # Use varying prompt lengths to stress different cache/context scenarios
                prompt_size = random.randint(10, 100)
                prompt = "Explain the importance of " + " ".join(["security"] * prompt_size)

                response = await self.adapter.complete(
                    messages=[{"role": "user", "content": prompt}],
                    max_tokens=random.randint(50, 200),
                    temperature=0.7,
                    stop=None
                )

                latency = time.perf_counter() - start_time
                self.latencies.append(latency)
                self.results.append(len(response))
                _log.info(f"Worker {worker_id} finished request {req_id} in {latency:.2f}s ({len(response)} chars)")
            except Exception as e:
                self.errors += 1
                _log.error(f"Worker {worker_id} failed request {req_id}: {e}")
            finally:
                queue.task_done()

    async def execute(self):
        _log.info(f"--- STARTING STRESS TEST ---")
        _log.info(f"Model: {self.config.model_id}")
        _log.info(f"Concurrency: {self.concurrency}")
        _log.info(f"Total Requests: {self.total_requests}")

        try:
            # Ensure server is up
            await self.adapter._ensure_server()

            queue = asyncio.Queue()
            for i in range(self.total_requests):
                queue.put_nowait(i)

            start_test = time.perf_counter()
            workers = [asyncio.create_task(self.run_worker(i, queue)) for i in range(self.concurrency)]

            await asyncio.gather(*workers)
            total_duration = time.perf_counter() - start_test

            self.report(total_duration)
        finally:
            await self.adapter.close()

    def report(self, duration):
        _log.info(f"--- STRESS TEST REPORT ---")
        _log.info(f"Total Duration: {duration:.2f}s")
        _log.info(f"Successful Requests: {len(self.latencies)}")
        _log.info(f"Failed Requests: {self.errors}")

        if self.latencies:
            _log.info(f"Avg Latency: {statistics.mean(self.latencies):.2f}s")
            _log.info(f"Min Latency: {min(self.latencies):.2f}s")
            _log.info(f"Max Latency: {max(self.latencies):.2f}s")
            _log.info(f"P95 Latency: {statistics.quantiles(self.latencies, n=20)[18]:.2f}s")

            # Simple throughput estimate (characters per second as proxy for tokens if no real token count)
            total_chars = sum(self.results)
            _log.info(f"Throughput: {total_chars / duration:.2f} chars/sec")
        _log.info(f"--------------------------")

import argparse

async def main():
    parser = argparse.ArgumentParser(description="Llama.cpp Stress Tester")
    parser.add_argument("--model", type=str, help="Model ID from config or path")
    parser.add_argument("--path", type=str, help="Full path to GGUF model")
    parser.add_argument("--concurrency", type=int, default=4, help="Number of concurrent workers")
    parser.add_argument("--requests", type=int, default=20, help="Total requests to run")
    args = parser.parse_args()

    model_path = args.path or os.getenv("GGUF_MODEL_PATH")

    if not model_path and not args.model:
        # Fallback to the newly downloaded abliterated model
        model_path = "/Users/Cheraxxx/Interlock IDE/data/models/llama-3.1-8b-abliterated.Q4_K_M.gguf"
        _log.info(f"Using default model path: {model_path}")

    config = ModelConfig(
        model_id=args.model or "stress-test-abliterated",
        path=model_path,
        backend="llama_cpp",
        context_length=8192,
        vram_gb=16.0, # Optimized for M4 Pro
        quantization="Q4_K_M",
        temperature=0.7,
        top_p=0.9,
        repeat_penalty=1.1,
        server_host="127.0.0.1",
        server_port=8899,
        rdt_factor=1
    )

    tester = LlamaStressTester(config, concurrency=args.concurrency, total_requests=args.requests)
    await tester.execute()

if __name__ == "__main__":
    asyncio.run(main())
