from zeroconf import Zeroconf, ServiceInfo
import socket
import time

def simulate_mdns_device(service_name="iAmAVario2", service_type="_http._tcp.local.", port=8080):
    # Service type should be _http._tcp. without the local suffix
    ip_address = socket.gethostbyname(socket.getfqdn())
    # Service information
    desc = {'path': '/status'}
    info = ServiceInfo(
        service_type,
        f"{service_name}.{service_type}",
        addresses=[socket.inet_aton(ip_address)],
        port=port,
        properties=desc,
        server=f"{service_name}.local."
    )

    # Start broadcasting the service
    zeroconf = Zeroconf()
    print(f"Registering service {service_name} at {ip_address}:{port}")
    zeroconf.register_service(info)

    try:
        while True:
            print(f"{service_name} is running at {ip_address}:{port}")
            time.sleep(5)  # Keep the service alive
    except KeyboardInterrupt:
        print("Service interrupted. Shutting down...")
    finally:
        zeroconf.unregister_service(info)
        zeroconf.close()

if __name__ == "__main__":
    simulate_mdns_device()
