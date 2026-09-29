# ESP32-C5 Lichess Handheld Simulator v5.4

## Network latency measurement fix

v5.3 had a diagnostic bug: both the "cold" and "warm" probes used `stream=True`,
then closed the tiny response without consuming it. That can prevent the HTTP
connection from being returned to urllib3's pool, so the second request may perform
another TCP/TLS setup and falsely look like another cold request.

v5.4:

- fully consumes each tiny API response;
- uses `Response.elapsed` for response-header timing;
- performs 3 requests and reports the first as cold and the fastest subsequent sample as warm;
- shows whether Python Requests is using a proxy;
- separately measures a `trust_env=False` direct connection for comparison;
- keeps the existing real move POST and Board API stream-sync measurements.

This makes it much easier to distinguish:

- local Wi-Fi problems;
- proxy/VPN route latency;
- DNS/TCP/TLS cold-start cost;
- actual Lichess API response latency;
- Board API event-stream latency.
