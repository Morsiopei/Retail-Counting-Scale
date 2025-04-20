from flask import request, jsonify, current_app
import datetime

from . import bp # Import the blueprint instance from __init__.py
# from .schemas import ReadingSchema # Uncomment if using Marshmallow schemas
from ...services import data_handler # Import service functions

# --- Schema Instance (if using Marshmallow) ---
# reading_schema = ReadingSchema()
# readings_schema = ReadingSchema(many=True)


@bp.route('/reading', methods=['POST'])
def receive_reading():
    """
    Endpoint for scales to push data readings.
    Validates input JSON and passes it to the data handler service.
    """
    if not request.is_json:
        current_app.logger.warning("Received non-JSON request to /reading")
        return jsonify({"error": "Request must be JSON"}), 400

    data = request.get_json()
    current_app.logger.debug(f"Received data: {data}")

    # --- Validation ---
    # Option 1: Simple Manual Validation
    required_fields = ["device_id", "weight_grams", "item_count", "is_stable", "is_overload", "mode"]
    errors = {field: f"Missing required field: {field}" for field in required_fields if field not in data}
    if errors:
        current_app.logger.warning(f"Validation failed: {errors}")
        return jsonify({"errors": errors}), 400

    # Option 2: Using Marshmallow Schema (Uncomment relevant lines above)
    # try:
    #     validated_data = reading_schema.load(data)
    # except ValidationError as err:
    #     current_app.logger.warning(f"Schema validation failed: {err.messages}")
    #     return jsonify(err.messages), 400
    # data = validated_data # Use validated/cleaned data

    # --- Process Data via Service Layer ---
    try:
        success, message = data_handler.process_and_store_reading(data)
        if success:
            current_app.logger.info(f"Reading processed successfully for device {data.get('device_id')}")
            return jsonify({
                "message": message or "Reading received successfully",
                "device_id": data.get("device_id"),
                "received_timestamp": datetime.datetime.utcnow().isoformat() + 'Z'
            }), 201 # 201 Created
        else:
             current_app.logger.error(f"Failed to process reading: {message}")
             return jsonify({"error": message or "Failed to process reading"}), 500
    except Exception as e:
        current_app.logger.exception("Unhandled exception processing reading!") # Logs exception info
        return jsonify({"error": "Internal server error"}), 500


@bp.route('/readings/<string:device_id>', methods=['GET'])
def get_readings(device_id):
    """
    Example: Get recent readings for a specific device.
    Retrieves data via the service layer.
    """
    current_app.logger.info(f"Request received for readings from device: {device_id}")
    try:
        limit = request.args.get('limit', default=20, type=int)
        if limit > 100: # Add a reasonable upper bound
            limit = 100

        readings = data_handler.get_device_readings(device_id, limit=limit)

        # --- Serialize with Marshmallow (if used) ---
        # return jsonify(readings_schema.dump(readings)), 200

        # --- Simple JSON serialization ---
        return jsonify(readings), 200

    except Exception as e:
         current_app.logger.exception(f"Error retrieving readings for device {device_id}")
         return jsonify({"error": "Internal server error"}), 500

# --- Add more routes as needed ---
# Example: Route to get device status
# @bp.route('/status/<string:device_id>', methods=['GET'])
# def get_device_status(device_id):
#     # ... call data_handler.get_status(device_id) ...
#     pass

# Example: Route to send a command (e.g., request tare)
# @bp.route('/command/<string:device_id>', methods=['POST'])
# def send_command(device_id):
#     # ... get command from request json ...
#     # ... call data_handler.queue_command(device_id, command) ...
#     # This would likely involve an IoT platform or other mechanism
#     # to send the command down to the device.
#     pass
