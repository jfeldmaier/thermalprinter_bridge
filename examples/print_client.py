#!/usr/bin/env python3
"""
Example Python client for ESP32 Thermal Printer Gateway
Demonstrates how to send print jobs via WiFi
"""

import requests
import sys
from datetime import datetime

# Gateway configuration
GATEWAY_IP = "192.168.4.1"  # Default AP mode IP
# GATEWAY_IP = "192.168.1.100"  # Example station mode IP

BASE_URL = f"http://{GATEWAY_IP}"


def check_status():
    """Check the gateway and printer status"""
    try:
        response = requests.get(f"{BASE_URL}/status", timeout=5)
        response.raise_for_status()
        status = response.json()
        
        print("Gateway Status:")
        print(f"  WiFi: {status['wifi']}")
        print(f"  Bluetooth: {'Connected' if status['btConnected'] else 'Disconnected'}")
        print(f"  Free Heap: {status['freeHeap']} bytes")
        print(f"  Uptime: {status['uptime']} seconds")
        
        return status['btConnected']
    except requests.exceptions.RequestException as e:
        print(f"Error checking status: {e}")
        return False


def print_text(text):
    """Send text to the printer"""
    try:
        response = requests.post(
            f"{BASE_URL}/print",
            data=text,
            headers={'Content-Type': 'text/plain'},
            timeout=10
        )
        response.raise_for_status()
        result = response.json()
        
        print(f"Print job sent successfully!")
        print(f"  Bytes sent: {result.get('bytes', 'unknown')}")
        return True
    except requests.exceptions.RequestException as e:
        print(f"Error sending print job: {e}")
        return False


def print_receipt_example():
    """Print a sample receipt"""
    receipt = f"""
{'='*32}
    EXAMPLE RECEIPT
{'='*32}

Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

Items:
  Coffee           $2.50
  Croissant        $3.00
  Orange Juice     $2.00
              ----------
  Total:           $7.50

Thank you for your visit!

{'='*32}



"""
    return print_text(receipt)


def interactive_mode():
    """Interactive mode for testing"""
    print("\n=== ESP32 Thermal Printer Gateway Client ===\n")
    
    while True:
        print("\nOptions:")
        print("  1. Check status")
        print("  2. Print custom text")
        print("  3. Print sample receipt")
        print("  4. Exit")
        
        choice = input("\nEnter choice (1-4): ").strip()
        
        if choice == "1":
            check_status()
        elif choice == "2":
            print("\nEnter text to print (press Ctrl+D or Ctrl+Z when done):")
            try:
                lines = []
                while True:
                    line = input()
                    lines.append(line)
            except EOFError:
                text = "\n".join(lines) + "\n\n\n"
                print_text(text)
        elif choice == "3":
            print_receipt_example()
        elif choice == "4":
            print("Goodbye!")
            break
        else:
            print("Invalid choice. Please try again.")


def main():
    """Main function"""
    if len(sys.argv) > 1:
        # Command line argument provided
        if sys.argv[1] == "status":
            check_status()
        elif sys.argv[1] == "receipt":
            print_receipt_example()
        elif sys.argv[1] == "text":
            if len(sys.argv) > 2:
                text = " ".join(sys.argv[2:]) + "\n\n\n"
                print_text(text)
            else:
                print("Usage: python3 print_client.py text <message>")
        else:
            print(f"Usage: {sys.argv[0]} [status|receipt|text <message>]")
    else:
        # Interactive mode
        interactive_mode()


if __name__ == "__main__":
    main()
