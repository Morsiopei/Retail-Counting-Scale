from flask import Flask
# --- Uncomment extensions if you use them ---
# from flask_sqlalchemy import SQLAlchemy
# from flask_migrate import Migrate
# from flask_marshmallow import Marshmallow

from .config import config # Import the config dictionary

# --- Instantiate extensions (globally accessible) ---
# db = SQLAlchemy()
# migrate = Migrate()
# ma = Marshmallow()

# --- Application Factory ---
def create_app(config_name='default'):
    """Creates and configures the Flask application instance."""
    app = Flask(__name__, instance_relative_config=False) # instance_relative_config=False is often safer

    # Load configuration from config.py based on config_name
    app.config.from_object(config[config_name])
    config[config_name].init_app(app) # Optional: if config has an init_app method

    # --- Initialize Flask extensions ---
    # db.init_app(app)
    # migrate.init_app(app, db) # Requires db
    # ma.init_app(app)
    print(f"App created with config: {config_name}")
    if app.config.get('SQLALCHEMY_DATABASE_URI'):
         print(f"Database URI: {app.config['SQLALCHEMY_DATABASE_URI']}")


    # --- Register Blueprints ---
    # API v1
    from .api.v1 import bp as api_v1_blueprint
    app.register_blueprint(api_v1_blueprint, url_prefix='/api/v1')

    # --- Add a simple health check route ---
    @app.route('/health')
    def health_check():
        return "OK", 200

    # --- Shell Context Processor (optional, useful for `flask shell`) ---
    @app.shell_context_processor
    def make_shell_context():
        return dict(app=app) # Add db, models here if using them: dict(app=app, db=db, Reading=Reading)


    return app
