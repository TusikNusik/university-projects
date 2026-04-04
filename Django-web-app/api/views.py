from rest_framework import generics, permissions
from rest_framework.authentication import TokenAuthentication
from django.shortcuts import get_object_or_404

from trasy.models import PointList

from .serializer import PointListSerializer, PointSerializer

class PointListCreateView(generics.ListCreateAPIView):
    serializer_class = PointListSerializer
    authentication_classes = [TokenAuthentication]
    permission_classes = [permissions.IsAuthenticated]

    def get_queryset(self):
        return PointList.objects.filter(user=self.request.user)

    def perform_create(self, serializer):
        serializer.save(user=self.request.user)

class PointListDetailDeleteView(generics.RetrieveDestroyAPIView):
    serializer_class = PointListSerializer
    authentication_classes = [TokenAuthentication]
    permission_classes = [permissions.IsAuthenticated]

    def get_queryset(self):
        return PointList.objects.filter(user=self.request.user)


class PointListView(generics.ListAPIView):
    serializer_class = PointSerializer
    authentication_classes = [TokenAuthentication]
    permission_classes = [permissions.IsAuthenticated]

    def get_queryset(self):
        route = get_object_or_404(PointList, id=self.kwargs["route_id"], user=self.request.user)
        return route.points.all()

class PointCreateView(generics.CreateAPIView):
    serializer_class = PointSerializer
    authentication_classes = [TokenAuthentication]
    permission_classes = [permissions.IsAuthenticated]

    def perform_create(self, serializer):
        route = get_object_or_404(PointList, id=self.kwargs["route_id"], user=self.request.user)
        point = serializer.save()
        route.points.add(point)

class PointDeleteView(generics.DestroyAPIView):
    serializer_class = PointSerializer
    authentication_classes = [TokenAuthentication]
    permission_classes = [permissions.IsAuthenticated]

    def get_object(self):
        route = get_object_or_404(PointList, id=self.kwargs["route_id"], user=self.request.user)
        return get_object_or_404(route.points.all(), id=self.kwargs["point_id"])
