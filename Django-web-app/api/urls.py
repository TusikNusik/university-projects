from django.urls import path
from drf_spectacular.views import SpectacularAPIView, SpectacularSwaggerView
from rest_framework.authtoken.views import obtain_auth_token

from . import views

urlpatterns = [
    path('route/', views.PointListCreateView.as_view(), name='point-list-create'),
    path('route/<int:pk>/', views.PointListDetailDeleteView.as_view(), name='pointlist-detail-delete'),
    path('route/<int:route_id>/punkty/', views.PointListView.as_view(), name='point-list'),
    path('route/<int:route_id>/punkty/new/', views.PointCreateView.as_view(), name='point-create'),
    path('route/<int:route_id>/punkty/<int:point_id>/delete/', views.PointDeleteView.as_view(), name='point-delete'),
]

urlpatterns += [
    path('token/', obtain_auth_token),
]

urlpatterns += [
    path("schema/", SpectacularAPIView.as_view(), name="schema"),
    path("swagger/", SpectacularSwaggerView.as_view(url_name="schema"), name="swagger-ui"),
]
