from django.test import TestCase, Client
from django.contrib.auth.models import User
from django.urls import reverse
from trasy.models import Point, PointList, BackgroundImage

class WebInterfaceTests(TestCase):
    def setUp(self):
        self.client = Client()
        self.user = User.objects.create_user(username='maslo', password='qwerty123')
        self.background = BackgroundImage.objects.create(name="testImage", image="temp.png")
        self.pointList = PointList.objects.create(user=self.user, name="testList", backgroundImage=self.background)
        self.point = Point.objects.create(x=10, y=20, included=True)
        self.pointList.points.add(self.point)

    def test_user_has_set_values(self):
        self.client.login(username='maslo', password='qwerty123')
        self.assertTrue(PointList.objects.filter(name='testList').exists())
        self.assertTrue(BackgroundImage.objects.filter(name='testImage').exists())
        self.assertEqual(self.pointList.points.get(id=1).x, 10)

    def test_add_point_to_list(self):
        self.client.login(username='maslo', password='qwerty123')
        response = self.client.post(reverse('addPoint', args=[self.pointList.id]), {'addPoint': 'true', 'X-cord': '30', 'Y-cord': '40'})
        self.assertEqual(response.status_code, 200)
        self.assertEqual(self.pointList.points.count(), 2)

    def test_exclude_points(self):
        self.client.login(username='maslo', password='qwerty123')
        response = self.client.post(reverse('addPoint', args=[self.pointList.id]), { 'save': 'true', f'{self.point.id}': 'excluded' })
        self.point.refresh_from_db()
        self.assertFalse(self.point.included)
        response2 = self.client.post(reverse('addPoint', args=[self.pointList.id]), { 'save': 'true', f'{self.point.id}': 'included' })
        self.point.refresh_from_db()
        self.assertTrue(self.point.included)
        response3 = self.client.get(reverse('addPoint', args=[self.pointList.id]))
        html = response3.content.decode('utf-8')
        expected_input = f'name="{self.point.id}" checked'
        self.assertIn(expected_input, html)

    def test_saved_projects_view_only_own_lists(self):
        other_user = User.objects.create_user(username='otheruser', password='qwerty123')
        other_list = PointList.objects.create(user=other_user, name="otherList", backgroundImage=self.background)
        self.client.login(username='maslo', password='qwerty123')
        response = self.client.get(reverse('savedProjects'))
        self.assertEqual(response.status_code, 200)
        self.assertContains(response, "testList")
        self.assertNotContains(response, "otherList")
        self.client.logout()
        self.client.login(username='otheruser', password='qwerty123')
        response = self.client.get(reverse('savedProjects'))
        self.assertContains(response, "otherList")
        self.assertNotContains(response, "testList")

    def test_add_list_requires_login(self):
        response = self.client.get(reverse('savedProjects'))
        self.assertEqual(response.status_code, 302) 
        self.assertIn('/login?next=/', response.url)
