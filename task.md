# Task List

- [x] Modify `conn_mgr.h` to change default `consecutive_fail_to_mark_bad` to `2`.
- [x] Modify `firmware_node.h` to add inflight getters and update `upload_execute` signature.
- [x] Modify `firmware_node.cpp` to enforce gateway remaining capacity in Mesh uploads.
- [x] Modify `simulator.cpp` to update Phase 6 logic using inflight state and remaining capacities.
- [x] Modify `test_all.cpp` to add the `mesh_gateway_capacity_limit_enforced` test.
- [x] Compile and run all tests to verify correctness.
- [x] Modify `conn_mgr.h`/`cpp` to implement throughput-aware dynamic scoring.
- [x] Modify `firmware_node.cpp` & `simulator.cpp` to pass backlog and production rate.
- [x] Modify `test_all.cpp` to add the `throughput_aware_failover_prevents_loss` test.
- [x] Compile and run all tests to verify correctness.
- [x] Update `WRITEUP.md` to document the throughput-aware optimization.
