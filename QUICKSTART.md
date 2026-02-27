# Quick Start Guide - Image Printing

## For End Users (Web Interface)

1. **Connect to Gateway**
   - Connect to WiFi: `ThermalPrinter_GW` (password: `Print$2026!`)
   - Open browser: `http://192.168.4.1`

2. **Print an Image**
   - Scroll to "Print Image" section
   - Click "Choose Image File"
   - Select your image (JPG, PNG, etc.)
   - Preview shows black & white conversion
   - Click "Print Image"

3. **Tips for Best Results**
   - Use high-contrast images
   - Keep width under 384 pixels
   - Simple graphics work better than photos

## For Developers (Python Script)

```bash
# Install dependencies
pip install Pillow requests

# Print an image
python3 examples/print_image.py logo.png

# Print with custom width
python3 examples/print_image.py photo.jpg 320
```

## For Developers (API)

```bash
# Using curl with a pre-converted image
curl -X POST http://192.168.4.1/printImage \
  -H "Content-Type: application/json" \
  -d @image_data.json
```

Where `image_data.json` contains:
```json
{
  "data": "base64-encoded-bitmap-data-here",
  "width": 384,
  "height": 200
}
```

## Testing Without Hardware

Open `examples/test_image_printing.html` in your browser to:
- Test image conversion
- See how images look in black & white
- Generate test patterns
- Export bitmap data

## Common Issues

**Image too wide**: Resize to 384 pixels or less  
**Poor quality**: Increase contrast in image editor first  
**Printer not responding**: Check Bluetooth connection  
**"Printer not connected" error**: Pair printer via Bluetooth first

## Next Steps

See full documentation in:
- `README.md` - Complete user guide
- `examples/README.md` - All example scripts
- `IMPLEMENTATION.md` - Technical details
