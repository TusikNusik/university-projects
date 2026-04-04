from rest_framework import serializers

from trasy.models import Point, PointList

class PointListSerializer(serializers.ModelSerializer):
    class Meta:
        model = PointList
        fields = ["id", "user", "name", "backgroundImage", "points"]
        read_only_fields = ["user"]


class PointSerializer(serializers.ModelSerializer):
    class Meta:
        model = Point
        fields = ["id", "x", "y", "included"]
