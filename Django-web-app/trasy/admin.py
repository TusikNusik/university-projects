from django.contrib import admin

from .models import Point
from .models import PointList
from .models import BackgroundImage
from .models import GameBoard
from .models import Path

admin.site.register(Point)
admin.site.register(PointList)
admin.site.register(BackgroundImage)
admin.site.register(GameBoard)
admin.site.register(Path)