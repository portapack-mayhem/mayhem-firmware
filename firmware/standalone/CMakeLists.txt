cmake_minimum_required(VERSION 3.5)

project(standalone_apps)

add_subdirectory(pacman)

add_custom_target(
	standalone_apps
	DEPENDS pacman_app
)

