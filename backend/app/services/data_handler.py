from flask import current_app
import datetime
import json

# --- Uncomment if using SQLAlchemy models ---
# from .. import db
# from ..models.reading import Reading
# from sqlalchemy.exc import SQLAlchemyError


# --- Placeholder Data Store (Replace with actual DB logic) ---
# WARNING: This in-memory store is NOT persistent and NOT suitable for production!
IN_MEMORY_READINGS_STORE = {} # Dictionary where keys are device_ids

def process_and_store_reading(data):
    """
    Processes incoming reading data and stores it.
    Returns (True, "Success message") or (False, "Error message").
    """
    device_id = data.get("device_id")
    current_app.logger.debug(f"Processing reading for device: {device_id}")

    try:
        # --- Data Cleaning/Transformation (Example) ---
        # Attempt to parse timestamp if provided
        device_ts = None
        if data.get('timestamp'):
            try:
                # Handle different potential timestamp formats (e.g., ticks or ISO)
                if isinstance(data['timestamp'], (int, float)): # Assume ticks if numeric
                     device_ts = datetime.datetime.utcfromtimestamp(data['timestamp'] / 1000.0) # Assuming ms ticks
                else: # Assume ISO format string
                     device_ts = datetime.datetime.fromisoformat(str(data['timestamp']).replace('Z
