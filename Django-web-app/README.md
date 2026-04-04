# Route Planner

Route Planner is a Django web application for creating point lists on top of background images, designing board-based puzzles, and saving paths drawn across those boards.

The project combines a classic Django web interface with a small REST API. The browser side uses TypeScript for the interactive map, board editor, and path drawing screens.

## Overview

The application supports two main workflows:

- Map workflow: upload a background image, create a point list, and mark coordinates on top of the image.
- Board workflow: create a board, place colored dots, and save a path across the grid.

In addition to the HTML interface, the project exposes token-protected API endpoints for route and point management.

## Features

- User registration and login
- Point list management with background images
- Interactive point selection on an image canvas
- Board creation and editing with colored dots
- Saved path drawing for boards
- Token-authenticated REST API for route and point management

## Tech Stack

- Python
- Django
- Django REST Framework
- TypeScript
- SQLite

## Project Structure

- `trasy/` contains the main web application, templates, styles, models, and tests
- `api/` contains the REST API endpoints and serializers
- `register/` contains the registration flow
- `mysite/` contains the Django project configuration
- `typescript/` contains the TypeScript source files
- `static/js/dist/` contains the compiled browser scripts

## Requirements

- Python 3.12+ recommended
- Node.js and npm

## Local Setup

1. Create and activate a virtual environment:

```bash
python3 -m venv venv
source venv/bin/activate
```

2. Install Python dependencies:

```bash
pip install -r requirements.txt
```

3. Install frontend dependencies:

```bash
npm install
```

4. Apply database migrations:

```bash
python manage.py migrate
```

5. Build the TypeScript assets:

```bash
npm run build
```

6. Create a superuser if you want access to Django admin:

```bash
python manage.py createsuperuser
```

7. Start the development server:

```bash
python manage.py runserver
```

Open `http://127.0.0.1:8000/` in your browser.

## Main Routes

- `/` redirects into the main application flow
- `/route/` main interface
- `/register/` user registration
- `/login` and `/logout/` authentication
- `/admin/` Django admin
- `/api/` REST API

## Running Tests

Run the Django and API test suite with:

```bash
python manage.py test
```

If your virtual environment is outside the project directory, call the interpreter directly. Example:

```bash
../venv/bin/python manage.py test
```

## Frontend Build

The browser-side code is authored in `typescript/` and compiled into `static/js/dist/`.

To rebuild the frontend assets:

```bash
npm run build
```

## API Notes

The API is available under `/api/` and uses token authentication.

Important endpoints:

- `POST /api/token/` to obtain an auth token
- `GET /api/route/` to list the authenticated user's routes
- `POST /api/route/` to create a route
- `GET /api/route/<id>/punkty/` to list points in a route
- `POST /api/route/<id>/punkty/new/` to create a point in a route
- `DELETE /api/route/<id>/punkty/<point_id>/delete/` to remove a point
- `GET /api/schema/` for the OpenAPI schema
- `GET /api/swagger/` for Swagger UI

## Repository Notes

- Local database and uploaded media are ignored by Git via `.gitignore`.
- Compiled JavaScript is stored in `static/js/dist/`.
- The project currently uses SQLite for local development.