/**
 * Node.js client example for ESP32 Thermal Printer Gateway
 * 
 * Install dependencies:
 *   npm install axios
 * 
 * Usage:
 *   node print_client.js status
 *   node print_client.js print "Hello World"
 */

const axios = require('axios');

// Gateway configuration
const GATEWAY_IP = process.env.GATEWAY_IP || '192.168.4.1';
const BASE_URL = `http://${GATEWAY_IP}`;

/**
 * Check gateway and printer status
 */
async function checkStatus() {
  try {
    const response = await axios.get(`${BASE_URL}/status`);
    const status = response.data;
    
    console.log('Gateway Status:');
    console.log(`  WiFi: ${status.wifi}`);
    console.log(`  Bluetooth: ${status.btConnected ? 'Connected' : 'Disconnected'}`);
    console.log(`  Free Heap: ${status.freeHeap} bytes`);
    console.log(`  Uptime: ${status.uptime} seconds`);
    
    return status.btConnected;
  } catch (error) {
    console.error('Error checking status:', error.message);
    return false;
  }
}

/**
 * Send text to the printer
 */
async function printText(text) {
  try {
    // Add extra line feeds for paper advance
    const printData = text + '\n\n\n';
    
    const response = await axios.post(
      `${BASE_URL}/print`,
      printData,
      {
        headers: { 'Content-Type': 'text/plain' }
      }
    );
    
    console.log('✓ Print job sent successfully!');
    console.log(`  Bytes sent: ${response.data.bytes || 'unknown'}`);
    return true;
  } catch (error) {
    console.error('✗ Error sending print job:', error.message);
    if (error.response) {
      console.error('  Response:', error.response.data);
    }
    return false;
  }
}

/**
 * Print a sample receipt
 */
async function printReceipt() {
  const now = new Date();
  const receipt = `
${'='.repeat(32)}
    EXAMPLE RECEIPT
${'='.repeat(32)}

Date: ${now.toLocaleString()}

Items:
  Coffee           $2.50
  Croissant        $3.00
  Orange Juice     $2.00
              ----------
  Total:           $7.50

Thank you for your visit!

${'='.repeat(32)}


`;
  
  return await printText(receipt);
}

/**
 * Main function
 */
async function main() {
  const args = process.argv.slice(2);
  
  if (args.length === 0) {
    console.log('Usage:');
    console.log('  node print_client.js status           - Check gateway status');
    console.log('  node print_client.js print <text>     - Print custom text');
    console.log('  node print_client.js receipt          - Print sample receipt');
    return;
  }
  
  const command = args[0];
  
  switch (command) {
    case 'status':
      await checkStatus();
      break;
      
    case 'print':
      if (args.length < 2) {
        console.error('Error: No text provided');
        console.log('Usage: node print_client.js print <text>');
        process.exit(1);
      }
      const text = args.slice(1).join(' ');
      await printText(text);
      break;
      
    case 'receipt':
      await printReceipt();
      break;
      
    default:
      console.error(`Unknown command: ${command}`);
      console.log('Valid commands: status, print, receipt');
      process.exit(1);
  }
}

// Run main function
main().catch(error => {
  console.error('Unexpected error:', error);
  process.exit(1);
});

module.exports = { checkStatus, printText, printReceipt };
