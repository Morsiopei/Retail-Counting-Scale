from app import create_app
import os

# Create the Flask app instance using the factory
# It will load configuration based on environment (e.g., Config class from config.py)
application = create_app(os.getenv('FLASK_CONFIG') or 'default')

# You might add more production-specific setup here if needed,
# but usually Gunicorn/uWSGI handles the server aspects.
