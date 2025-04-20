# from app import ma # Import Marshmallow instance if created in app/__init__
from marshmallow import Schema, fields, validate, ValidationError

# Example Schema for incoming reading data
class ReadingSchema(Schema):
    device_id = fields.Str(required=True, validate=validate.Length(min=1))
    timestamp = fields.AwareDateTime(required=False, allow_none=True) # ISO 8601 Format expected if provided
    weight_grams = fields.Float(required=True)
    item_count = fields.Int(required=True)
    is_stable = fields.Bool(required=True)
    is_overload = fields.Bool(required=True)
    average_item_weight = fields.Float(required=False, allow_none=True)
    mode = fields.Str(required=True, validate=validate.OneOf(["WEIGHING", "COUNTING", "ERROR"]))

    # Example custom validation
    # @validates_schema
    # def validate_weight(self, data, **kwargs):
    #     if data.get('weight_grams', 0) < 0 and not data.get('is_overload'):
    #         raise ValidationError("Weight cannot be negative unless overloaded.")

# Note: You would typically instantiate this schema in routes.py
# reading_schema = ReadingSchema()
# readings_schema = ReadingSchema(many=True) # For lists of readings
