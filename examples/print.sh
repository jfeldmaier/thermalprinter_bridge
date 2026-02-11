#!/bin/bash
#
# Simple bash script to send print jobs to the thermal printer gateway
# Usage: ./print.sh "Text to print"
#

# Gateway IP address (change if using station mode)
GATEWAY_IP="${GATEWAY_IP:-192.168.4.1}"
GATEWAY_URL="http://${GATEWAY_IP}/print"

# Check if text is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <text to print>"
    echo "Example: $0 \"Hello World!\""
    exit 1
fi

# Text to print (add line feeds for paper advance)
TEXT="$1\n\n\n"

# Send print job
echo "Sending print job to ${GATEWAY_URL}..."
response=$(echo -e "${TEXT}" | curl -s -X POST \
    -H "Content-Type: text/plain" \
    --data-binary @- \
    "${GATEWAY_URL}")

# Check response
if echo "$response" | grep -q "success"; then
    echo "✓ Print job sent successfully!"
    echo "Response: $response"
else
    echo "✗ Failed to send print job"
    echo "Response: $response"
    exit 1
fi
