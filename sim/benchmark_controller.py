"""
AutoFlora Edge Computation Latency & Throughput Benchmark.
"""
import time
from sim.resilience_watchdog import IrrigationResilienceSupervisor

def run_benchmark():
    sup = IrrigationResilienceSupervisor(window_size=11)
    iterations = 50_000
    
    start = time.perf_counter()
    for i in range(iterations):
        val = 1200.0 + (i % 50)
        sup.add_sample(val)
        if i % 100 == 0:
            sup.open_valve(current_time=float(i))
            sup.check_watchdog(current_time=float(i) + 10.0)
            sup.close_valve()
    end = time.perf_counter()
    
    elapsed = end - start
    ops_per_sec = iterations / elapsed
    print(f"Benchmark: {iterations} cycles in {elapsed:.4f}s ({ops_per_sec:,.0f} ops/sec)")
    print(f"Average loop latency: {(elapsed / iterations) * 1e6:.2f} microseconds")

if __name__ == '__main__':
    run_benchmark()
