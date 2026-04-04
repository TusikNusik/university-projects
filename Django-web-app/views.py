from django.contrib.auth.decorators import login_required
from django.shortcuts import render, get_object_or_404, redirect
from .models import Point
from .models import PointList
from .forms import FormPoint
from .forms import FormList
from .forms import FormBoard
from .models import GameBoard
from .models import Dot
import json
from django.http import JsonResponse

def index(request):
    return render(request, "trasy/base.html")

def points(request):
    pointList = Point.objects.all()

    pointDictonary = {"pointList": pointList}

    return render(request, "trasy/pointList.html", pointDictonary)

def addPoint(request, id):
    point_list = PointList.objects.get(id=id)
    dict = request.POST
    
    if request.method == "POST":
        if dict.get("save"):
            for point in point_list.points.all():
                if dict.get(f"{point.id}") == "included":
                    point.included = True
                else:
                    point.included = False
                
                point.save()

        elif dict.get("addPoint"):
            x = dict.get("X-cord")
            y = dict.get("Y-cord")


            temp = Point(x=x, y=y, included=True)
            temp.save()
            point_list.points.add(temp)

    included_points = list(point_list.points.filter(included=True).values('x', 'y'))

    return render(request, "trasy/addPoint.html", {"list": point_list, "included_points": included_points})

def addList(request):
    if request.method == "POST":
        form = FormList(request.POST, request.FILES)

        if form.is_valid():
            point_list = form.save(commit=False)
            point_list.user = request.user
            point_list.save()

    form = FormList()
    return render(request, "trasy/addList.html", {"form": form})

@login_required
def savedProjects(request):
    user_lists = PointList.objects.filter(user=request.user)
    return render(request, "trasy/savedProjects.html", {"user_lists": user_lists})

@login_required
def savedBoards(request):
    user_boards = GameBoard.objects.filter(user=request.user)
    return render(request, "trasy/savedBoards.html", {"user_lists": user_boards})

@login_required
def addBoard(request):
    if request.method == "POST":
        form = FormBoard(request.POST, request.FILES)

        if form.is_valid():
            board = form.save(commit=False)
            board.user = request.user
            board.tabel = {}
            board.save()

    form = FormBoard()

    return render(request, "trasy/addBoard.html", {"form": form})



@login_required
def editBoard(request, id):
    board = get_object_or_404(GameBoard, id=id, user=request.user)

    if request.method == "POST":
        data = json.loads(request.body)
        rows = data.get("rows")
        cols = data.get("cols")
        dots = data.get("dots", [])

        board.rows = rows
        board.cols = cols
        board.save()

        Dot.objects.filter(board=board).delete()
        for dot in dots:
            Dot.objects.create(
                board=board,
                row=dot["row"],
                col=dot["col"],
                color=dot["color"]
            )

        return JsonResponse({"success": True})

    dot_qs = Dot.objects.filter(board=board)
    dots = list(dot_qs.values("row", "col", "color"))

    return render(request, "trasy/editBoard.html", {
        "board": board,
        "dots_json": json.dumps(dots),
    })

def maslo(request):
    return render(request, "trasy/troll.html")
