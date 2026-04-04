from django.db import models
from django.contrib.auth.models import User


class BackgroundImage(models.Model):
    name = models.CharField(max_length=100)
    image = models.ImageField(upload_to='background/')

    def __str__(self):
        return self.name

class Point(models.Model):
    x = models.IntegerField()
    y = models.IntegerField()
    included = models.BooleanField(default=True)

    def __str__(self):
        return f"({self.x},{self.y})"

class PointList(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    name = models.CharField(max_length=100)
    backgroundImage = models.ForeignKey(BackgroundImage, on_delete=models.CASCADE, default=3)
    points = models.ManyToManyField(Point, related_name='point_lists')

    def __str__(self):
        return f"{self.name}"

class GameBoard(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    name = models.CharField(max_length=100)
    rows = models.PositiveIntegerField()
    cols = models.PositiveIntegerField()
    tabel = models.JSONField()

    def __str__(self):
        return self.name
    
class Dot(models.Model):
    board = models.ForeignKey(GameBoard, on_delete=models.CASCADE, related_name='dots')
    row = models.PositiveIntegerField()
    col = models.PositiveIntegerField()
    color = models.CharField()

    def __str__(self):
        return f"{self.board.name}: ({self.row}, {self.col})"

class Path(models.Model):
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    board = models.ForeignKey(GameBoard, on_delete=models.CASCADE)
    points = models.JSONField(default=list)

    def __str__(self):
        return f"{self.user.username} - {self.board.name}"
