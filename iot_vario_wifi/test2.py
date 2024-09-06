from zeroconf import ServiceBrowser, Zeroconf
import threading
import time

class MyListener:
    def __init__(self):
        self.discovered_devices = []

    def remove_service(self, zeroconf, service_type, name):
        print(f"Service {name} removed")

    def add_service(self, zeroconf, service_type, name):
        info = zeroconf.get_service_info(service_type, name)
        if info:
            ip_address = ".".join(map(str, info.addresses[0]))
            print(f"Service {name} added, IP: {ip_address}")
            self.discovered_devices.append((name, ip_address))

    def get_discovered_devices(self):
        return self.discovered_devices


# Function to start mDNS discovery
def discover_mdns(timeout=10):
    zeroconf = Zeroconf()
    listener = MyListener()
    browser = ServiceBrowser(zeroconf, "_http._tcp.local.", listener)

    # Allow discovery for the specified timeout
    time.sleep(timeout)
    
    # Stop discovery after timeout
    zeroconf.close()
    
    return listener.get_discovered_devices()


if __name__ == "__main__":
    print("Discovering services using mDNS...")
    devices = discover_mdns(timeout=10)
    print("\nDiscovered devices:")
    for name, ip in devices:
        print(f"Service: {name}, IP: {ip}")
