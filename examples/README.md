# Thermal Printer Gateway - Examples

This directory contains example scripts and tools for using the ESP32 Thermal Printer Gateway.

## Python Examples

### Prerequisites
```bash
pip install -r requirements.txt
```

### Text Printing
```bash
# Interactive mode
python3 print_client.py

# Check status
python3 print_client.py status

# Print text
python3 print_client.py text "Hello World"

# Print sample receipt
python3 print_client.py receipt
```

### Image Printing
```bash
# Print an image (default max width: 384 pixels)
python3 print_image.py logo.png

# Print with custom width
python3 print_image.py photo.jpg 320

# Print a QR code or barcode image
python3 print_image.py qrcode.png
```

## JavaScript/Node.js Examples

### Prerequisites
```bash
npm install axios
```

### Usage
```bash
# Check status
node print_client.js status

# Print text
node print_client.js print "Hello from Node.js"

# Print sample receipt
node print_client.js receipt
```

## Bash Script

### Usage
```bash
# Make executable
chmod +x print.sh

# Print text
./print.sh "Hello from Bash!"

# With custom gateway IP
GATEWAY_IP=192.168.1.100 ./print.sh "Custom IP"
```

## HTML Test Tool

Open `test_image_printing.html` in a web browser to:
- Test image conversion to thermal printer format
- Preview black & white conversion
- Generate test patterns
- Export bitmap data as JSON

This is useful for debugging and understanding the image format without needing the actual hardware.

## Image Format Notes

### Supported Input Formats
- JPG, PNG, GIF, BMP, and most common image formats
- The gateway/script will convert to 1-bit monochrome automatically

### Recommended Image Specifications
- **Width**: 384 pixels (standard thermal printer width)
  - Alternative widths: 320, 512, 576 pixels depending on printer model
- **Height**: No strict limit, but keep under 2000 pixels for best results
- **Contrast**: High contrast images work best
- **Content**: Simple line art, text, and logos work better than photos

### Image Preparation Tips

1. **For best results**:
   - Use high contrast images
   - Black text on white background works perfectly
   - Convert to grayscale first if using image editing software
   - Increase contrast and brightness as needed

2. **For photos**:
   - Increase contrast significantly
   - Consider applying edge detection or posterization
   - Test with small samples first
   - Results may vary depending on image complexity

3. **For logos and graphics**:
   - Vector graphics work well
   - Simple line art is ideal
   - Avoid gradients and subtle shading

## Troubleshooting

### Connection Issues
- Ensure the ESP32 gateway is powered on
- Check WiFi connection (default: ThermalPrinter_GW / printer123)
- Verify the gateway IP address (default: 192.168.4.1)
- Update `GATEWAY_IP` in scripts if using custom IP

### Image Printing Issues
- **Image too large**: Reduce width to 384 pixels or less
- **Poor quality**: Increase image contrast before converting
- **Missing details**: Simplify the image or increase size
- **Slow printing**: Large images take longer; be patient

### Python Issues
```bash
# If Pillow installation fails
pip install --upgrade pip
pip install Pillow

# If requests fails
pip install requests

# Check Python version (3.7+ recommended)
python3 --version
```

## License

These examples are provided under the same MIT license as the main project.
