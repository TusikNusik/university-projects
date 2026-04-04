from django.urls import path
from . import views

urlpatterns = [
    path("", views.index, name="index"),
    path("points", views.points, name="points"),
    path("addList", views.addList, name="addList"),
    path("addPoint/<int:id>", views.addPoint, name="addPoint"),
    path("savedProjects", views.savedProjects, name="savedProjects"),
    path("savedBoards", views.savedBoards, name="savedBoards"),
    path("addBoard", views.addBoard, name="addBoard"),
    path("editBoard/<int:id>", views.editBoard, name="editBoard"),
    path("allBoards", views.allBoards, name="allBoards"),
    path("drawPath/<int:id>", views.drawPath, name="drawPath"),
    path("savedPaths", views.savedPaths, name="savedPaths"),
    path("maslo", views.maslo, name="maslo"),
]
