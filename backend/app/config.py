import os
from dotenv import load_dotenv

# Determine the base directory of the project
basedir = os.path.abspath(os.path.dirname(__file__))
# Load environment variables from .env file located at the project root (backend/)
load_dotenv(os.path.join(basedir, '..', '.env'))


class Config:
    """Base configuration settings."""
    SECRET_KEY = os.environ.get('SECRET_KEY') or 'hard-to-guess-string-change-me'
    # --- Database ---
    SQLALCHEMY_TRACK_MODIFICATIONS = False # Suppress SQLAlchemy warning
    SQLALCHEMY_RECORD_QUERIES = True # Enable query recording (useful for debugging)
    SLOW_DB_QUERY_TIME = 0.5 # Threshold for logging slow queries (seconds)

    # --- Other common settings ---
    # MAIL_SERVER = os.environ.get('MAIL_SERVER')
    # MAIL_PORT = int(os.environ.get('MAIL_PORT') or 25)
    # MAIL_USE_TLS = os.environ.get('MAIL_USE_TLS') is not None
    # MAIL_USERNAME = os.environ.get('MAIL_USERNAME')
    # MAIL_PASSWORD = os.environ.get('MAIL_PASSWORD')
    # ADMIN_EMAIL = os.environ.get('ADMIN_EMAIL')

    @staticmethod
    def init_app(app):
        # Perform any initialization based on the base config
        pass

class DevelopmentConfig(Config):
    """Development specific configuration."""
    DEBUG = True
    # Use SQLite for simple development setup (file stored in backend/instance/app.db)
    # Ensure the 'instance' folder exists at the same level as the 'app' folder
    SQLALCHEMY_DATABASE_URI = os.environ.get('DEV_DATABASE_URL') or \
        'sqlite:///' + os.path.join(basedir, '..', 'instance', 'dev_app.db')
    print(f"Using DEV DB: {SQLALCHEMY_DATABASE_URI}")

class TestingConfig(Config):
    """Testing specific configuration."""
    TESTING = True
    SQLALCHEMY_DATABASE_URI = os.environ.get('TEST_DATABASE_URL') or \
        'sqlite://' # Use in-memory SQLite for tests by default
    WTF_CSRF_ENABLED = False # Disable CSRF forms protection in tests

class ProductionConfig(Config):
    """Production specific configuration."""
    DEBUG = False
    SQLALCHEMY_DATABASE_URI = os.environ.get('DATABASE_URL') # Must be set in environment
    # Add other production hardening settings: logging, security headers, etc.

    @classmethod
    def init_app(cls, app):
        Config.init_app(app)
        # Example: Log to stderr in production
        import logging
        from logging import StreamHandler
        handler = StreamHandler()
        handler.setLevel(logging.INFO)
        app.logger.addHandler(handler)


# Dictionary to access config classes by name
config = {
    'development': DevelopmentConfig,
    'testing': TestingConfig,
    'production': ProductionConfig,
    'default': DevelopmentConfig # Default config to use
}
