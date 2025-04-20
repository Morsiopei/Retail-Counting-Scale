from app import create_app
import os

# Create app using the factory, typically for development config
app = create_app(os.getenv('FLASK_CONFIG') or 'development')

if __name__ == '__main__':
    # Run the built-in Flask development server
    # Debug=True enables auto-reloading and interactive debugger (NEVER use in production)
    # Host='0.0.0.0' makes the server accessible on your network (use with caution)
    app.run(debug=True, host='0.0.0.0', port=5000)
