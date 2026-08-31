# Security and safety policy

AutoFlora controls a pump and should be operated as a low-voltage prototype, not a mains controller.

- Keep mains wiring outside this repository and have qualified personnel handle electrical work.
- Do not commit Wi-Fi credentials, deployment locations, camera images, or telemetry that identifies a site.
- Treat sensor readings and remote commands as untrusted; a failed sensor must not silently enable irrigation.
- Test relay isolation, tank-low protection, flow monitoring, and watchdog behavior before field use.
- Review any network gateway or cloud integration for authentication, replay, and unsafe actuator commands.

Report suspected credential exposure, actuator bypass, or unsafe firmware behavior privately to the repository owner with sanitized evidence.
