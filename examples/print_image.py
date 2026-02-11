#!/usr/bin/env python3
"""
Example Python client for printing images to ESP32 Thermal Printer Gateway
Demonstrates how to convert and send image files via WiFi
"""

import requests
import sys
import base64
from PIL import Image
import io

# Gateway configuration
GATEWAY_IP = "192.168.4.1"  # Default AP mode IP
BASE_URL = f"http://{GATEWAY_IP}"


def convert_image_to_bitmap(image_path, max_width=384):
    """
    Convert an image to 1-bit black and white bitmap
    
    Args:
        image_path: Path to the image file
        max_width: Maximum width in pixels (thermal printers typically use 384 or 576)
    
    Returns:
        tuple: (bitmap_data_base64, width, height)
    """
    # Open and convert image
    img = Image.open(image_path)
    
    # Convert to grayscale
    img = img.convert('L')
    
    # Resize if too wide
    width, height = img.size
    if width > max_width:
        height = int(height * (max_width / width))
        width = max_width
        img = img.resize((width, height), Image.Resampling.LANCZOS)
    
    print(f"Image size: {width}x{height} pixels")
    
    # Convert to 1-bit black and white
    img = img.point(lambda x: 0 if x < 128 else 255, '1')
    
    # Convert to bitmap bytes
    width_bytes = (width + 7) // 8
    bitmap_data = bytearray(width_bytes * height)
    
    pixels = img.load()
    for y in range(height):
        for x in range(width):
            if pixels[x, y] == 0:  # Black pixel
                byte_idx = y * width_bytes + (x // 8)
                bit_idx = 7 - (x % 8)
                bitmap_data[byte_idx] |= (1 << bit_idx)
    
    # Encode to base64
    bitmap_base64 = base64.b64encode(bytes(bitmap_data)).decode('ascii')
    
    return bitmap_base64, width, height


def print_image(image_path, max_width=384):
    """
    Send an image to the thermal printer
    
    Args:
        image_path: Path to the image file
        max_width: Maximum width in pixels
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        print(f"\nProcessing image: {image_path}")
        
        # Convert image to bitmap
        bitmap_data, width, height = convert_image_to_bitmap(image_path, max_width)
        
        print(f"Bitmap size: {len(bitmap_data)} bytes (base64 encoded)")
        
        # Prepare JSON payload
        payload = {
            "data": bitmap_data,
            "width": width,
            "height": height
        }
        
        print("Sending to printer...")
        
        # Send to gateway
        response = requests.post(
            f"{BASE_URL}/printImage",
            json=payload,
            timeout=30
        )
        response.raise_for_status()
        result = response.json()
        
        if result.get('success'):
            print(f"✓ Image printed successfully!")
            print(f"  Dimensions: {result.get('width')}x{result.get('height')} pixels")
            return True
        else:
            print(f"✗ Error: {result.get('error', 'Unknown error')}")
            return False
            
    except FileNotFoundError:
        print(f"✗ Error: Image file not found: {image_path}")
        return False
    except requests.exceptions.RequestException as e:
        print(f"✗ Error communicating with gateway: {e}")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False


def main():
    """Main function"""
    if len(sys.argv) < 2:
        print("Usage:")
        print(f"  {sys.argv[0]} <image_file> [max_width]")
        print("\nExamples:")
        print(f"  {sys.argv[0]} logo.png")
        print(f"  {sys.argv[0]} photo.jpg 320")
        print("\nSupported formats: PNG, JPG, GIF, BMP, etc.")
        print("Recommended max width: 384 pixels (thermal printer standard)")
        sys.exit(1)
    
    image_path = sys.argv[1]
    max_width = 384
    
    if len(sys.argv) > 2:
        try:
            max_width = int(sys.argv[2])
            if max_width <= 0 or max_width > 576:
                print("Warning: max_width should be between 1 and 576")
                max_width = min(max(max_width, 1), 576)
        except ValueError:
            print("Warning: Invalid max_width, using default (384)")
    
    success = print_image(image_path, max_width)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
