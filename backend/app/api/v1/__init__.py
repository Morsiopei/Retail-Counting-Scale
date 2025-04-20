from flask import Blueprint

# Create the Blueprint instance for API version 1
bp = Blueprint('api_v1', __name__)

# Import the routes module AFTER the blueprint is created
# This avoids circular dependencies as routes.py will import 'bp'
from . import routes, schemas # Schemas might be used by routes
