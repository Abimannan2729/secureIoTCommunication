import json
import hmac
import hashlib
from decimal import Decimal
import boto3
from time import time
from Crypto.Cipher import AES

# ---------------- Keys (must match ESP32) ----------------
AES_KEY = bytes([
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81
])
AES_IV = bytes([
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
])
HMAC_KEY = b"S3cUr3!HMACk3y1234"

# ---------------- DynamoDB ----------------
dynamodb = boto3.resource('dynamodb')
TABLE_NAME = "ESP32_DHT22_Data_New"
table = dynamodb.Table(TABLE_NAME)

# ---------------- Helper Functions ----------------
def verify_hmac(data_hex, hmac_received):
    """Verify HMAC-SHA256"""
    computed_hmac = hmac.new(HMAC_KEY, data_hex.encode('utf-8'), hashlib.sha256).hexdigest()
    return computed_hmac.lower() == hmac_received.lower()

def decrypt_aes_zero_padding(data_hex):
    """Decrypt AES-128-CBC with zero padding"""
    cipher = AES.new(AES_KEY, AES.MODE_CBC, AES_IV)
    encrypted_bytes = bytes.fromhex(data_hex)
    decrypted_bytes = cipher.decrypt(encrypted_bytes)
    decrypted_bytes = decrypted_bytes.rstrip(b'\x00')  # remove zero padding
    return decrypted_bytes.decode('utf-8')

# ---------------- Lambda Handler ----------------
def lambda_handler(event, context):
    payload = event.get("payload", {})
    data_hex = payload.get("data")
    hmac_received = payload.get("hmac")

    if not data_hex or not hmac_received:
        return {"status": "error", "message": "Missing data or hmac"}

    # Verify HMAC
    if not verify_hmac(data_hex, hmac_received):
        return {"status": "error", "message": "HMAC verification failed"}

    # Decrypt AES
    try:
        decrypted_json = decrypt_aes_zero_padding(data_hex)
        data = json.loads(decrypted_json)

        # Add timestamp
        timestamp = str(int(time()))

        # Convert floats to Decimal for DynamoDB
        item = {
            "timestamp": timestamp,
            "temperature": Decimal(str(data.get("temperature"))),
            "humidity": Decimal(str(data.get("humidity")))
        }

        # Write to DynamoDB
        table.put_item(Item=item)

        return {"status": "success", "decrypted_data": data, "db_item": item}

    except Exception as e:
        return {"status": "error", "message": f"Decryption or DynamoDB write failed: {str(e)}"}
