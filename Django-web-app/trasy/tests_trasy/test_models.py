from django.test import TestCase
from django.core.exceptions import ValidationError
from django.contrib.auth.models import User
from trasy.models import BackgroundImage, PointList, Point

class TestModels(TestCase):
    def setUp(self):
        self.user = User.objects.create_user(username="maslo")
        self.image = BackgroundImage.objects.create(name="map", image="temp.png")
        self.route = PointList.objects.create(user=self.user, backgroundImage=self.image, name="PointList1")

    def test_pointList_and_Point(self):
        temp1 = Point(x=4, y=3)
        temp2 = Point(x=7, y=2, included=False)
        temp1.save()
        temp2.save()
        self.route.points.add(temp1)
        self.assertEqual(self.route.points.count(), 1)
        self.route.points.add(temp2)
        self.assertEqual(self.route.points.count(), 2)
        self.assertEqual(self.route.backgroundImage.name, "map")
        self.assertEqual(self.route.points.get(id=1).x, 4)
        self.assertEqual(self.route.points.get(id=1).y, 3)
        self.assertEqual(self.route.points.get(id=1).included, True)
        self.assertEqual(self.route.points.get(id=2).x, 7)
        self.assertEqual(self.route.points.get(id=2).included, False)

    def test_backgroundImage(self):
        self.assertEqual(self.image.image, "temp.png")
        self.assertEqual(self.image.name, "map")

