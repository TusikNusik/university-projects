import json

from django.contrib.auth.decorators import login_required
from django.http import JsonResponse
from django.shortcuts import get_object_or_404, render

from .forms import FormBoard, FormList
from .models import Dot, GameBoard, Path, Point, PointList


def index(request):
    """Render the main application shell."""

    return render(request, "trasy/base.html")


def points(request):
    """Show all stored points."""

    point_list = Point.objects.all().order_by("id")
    return render(request, "trasy/pointList.html", {"pointList": point_list})


@login_required
def addPoint(request, id):
    """Add points to a list and toggle whether they are active."""

    point_list = get_object_or_404(PointList, id=id, user=request.user)

    if request.method == "POST":
        form_data = request.POST
        if form_data.get("save"):
            for point in point_list.points.all():
                point.included = form_data.get(str(point.id)) == "included"
                point.save()
        elif form_data.get("addPoint"):
            x_value = form_data.get("X-cord")
            y_value = form_data.get("Y-cord")
            try:
                x_coord = int(x_value) if x_value else None
                y_coord = int(y_value) if y_value else None
            except (TypeError, ValueError):
                x_coord = y_coord = None

            if x_coord is not None and y_coord is not None:
                point = Point(x=x_coord, y=y_coord, included=True)
                point.save()
                point_list.points.add(point)

    included_points = list(point_list.points.filter(included=True).values("x", "y"))
    return render(request, "trasy/addPoint.html", {"list": point_list, "included_points": included_points})


@login_required
def addList(request):
    """Create a new point list for the authenticated user."""

    form = FormList(request.POST or None, request.FILES or None)
    if request.method == "POST" and form.is_valid():
        point_list = form.save(commit=False)
        point_list.user = request.user
        point_list.save()
        form = FormList()

    return render(request, "trasy/addList.html", {"form": form})


@login_required
def savedProjects(request):
    """Display the current user's saved point lists."""

    user_lists = PointList.objects.filter(user=request.user).order_by("name")
    return render(request, "trasy/savedProjects.html", {"user_lists": user_lists})


@login_required
def savedBoards(request):
    """Display the current user's saved boards."""

    user_boards = GameBoard.objects.filter(user=request.user).order_by("name")
    return render(request, "trasy/savedBoards.html", {"user_lists": user_boards})


@login_required
def addBoard(request):
    """Create a new board for the authenticated user."""

    form = FormBoard(request.POST or None, request.FILES or None)
    if request.method == "POST" and form.is_valid():
        board = form.save(commit=False)
        board.user = request.user
        board.tabel = {}
        board.save()
        form = FormBoard()

    return render(request, "trasy/addBoard.html", {"form": form})


@login_required
def editBoard(request, id):
    """Edit a board grid and its colored dots."""

    board = get_object_or_404(GameBoard, id=id, user=request.user)

    if request.method == "POST":
        data = json.loads(request.body)
        board.rows = data.get("rows")
        board.cols = data.get("cols")
        board.save()

        Dot.objects.filter(board=board).delete()
        for dot in data.get("dots", []):
            Dot.objects.create(
                board=board,
                row=dot["row"],
                col=dot["col"],
                color=dot["color"],
            )

        return JsonResponse({"success": True})

    dots = list(Dot.objects.filter(board=board).values("row", "col", "color"))
    return render(request, "trasy/editBoard.html", {"board": board, "dots_json": json.dumps(dots)})


def allBoards(request):
    """Show every available board."""

    boards = GameBoard.objects.all().order_by("name")
    return render(request, "trasy/allBoards.html", {"boards": boards})


@login_required
def drawPath(request, id):
    """Create or update a saved path for a board."""

    board = get_object_or_404(GameBoard, pk=id)

    if request.method == "GET":
        dots = Dot.objects.filter(board=board)
        path = Path.objects.filter(board=board, user=request.user).first()
        return render(
            request,
            "trasy/drawPath.html",
            {
                "board": board,
                "dots_json": list(dots.values("row", "col", "color")),
                "saved_path": path.points if path else [],
            },
        )

    if request.method == "POST":
        try:
            data = json.loads(request.body)
            points = data.get("path", [])

            if not isinstance(points, list) or any("row" not in point or "col" not in point for point in points):
                return JsonResponse({"error": "Invalid data"}, status=400)

            Path.objects.update_or_create(
                user=request.user,
                board=board,
                defaults={"points": points},
            )
            return JsonResponse({"status": "ok"})
        except json.JSONDecodeError:
            return JsonResponse({"error": "Invalid JSON"}, status=400)

    return JsonResponse({"error": "Invalid method"}, status=405)


@login_required
def savedPaths(request):
    """Display saved paths for the current user."""

    user_lists = Path.objects.filter(user=request.user).select_related("board")
    return render(request, "trasy/savedPaths.html", {"user_lists": user_lists})


def maslo(request):
    """Render the legacy easter-egg page."""

    return render(request, "trasy/troll.html")
