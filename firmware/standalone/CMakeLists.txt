cmake_minimum_required(VERSION 3.16)

project(standalone_apps)

add_subdirectory(pacman)
add_subdirectory(digitalrain)
add_subdirectory(meteor_lrpt_decode)
add_subdirectory(meteor_lrpt_view)

add_custom_target(
	standalone_apps
	DEPENDS pacman_app digitalrain_app meteor_lrpt_decode_app meteor_lrpt_view_app
)

