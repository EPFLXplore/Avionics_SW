# Predefined positions of the arm
# HOME, ZERO and COBRA are predefined pose. If you want to use them, set the target = NAMED_POSE and predefined_pose = HOME for ex 

string                      NAMED_POSE = "named_pose"
string                      HOME = "home"
string                      ZERO = "zero"
string                      COBRA = "cobra"
string                      ABOVE_GROUND = "above_ground"
string                      RANGEMENT = "rangement"
string                      FRONT_PANEL = "front_panel"
string                      ALIGN_METAL_BAR = "align_metal_bar"

# Tasks to perform

# ----------------------------------------------------------------------------------------------

## Align with an object of the maintenance panel with aruco align task before
string                      ALIGN_OBJECT_WITH_ARUCO = "align_object_with_aruco"

string                      BUTTON_1 = "switch_1"
string                      BUTTON_2 = "switch_2"
string                      BUTTON_3 = "switch_3"
string                      BUTTON_4 = "switch_4"
string                      BUTTON_5 = "switch_5"
string                      BUTTON_6 = "switch_6"
string                      BUTTON_7 = "switch_7"
string                      BUTTON_8 = "switch_8"
string                      BUTTON_9 = "switch_9"
string                      BUTTON_10 = "switch_10"
string                      BUTTON_11 = "switch_11"
string                      BUTTON_12 = "switch_12"
string                      BUTTON_13 = "switch_13"
string                      BUTTON_MAIN = "switch_main"

string                      BIG_ROTATION_BUTTON_1 = "big_rotation_switch_1"
string                      BIG_ROTATION_BUTTON_2 = "big_rotation_switch_2"

string                      SMALL_ROTATION_BUTTON_1 = "small_rotation_switch_1"
string                      SMALL_ROTATION_BUTTON_2 = "small_rotation_switch_2"
string                      SMALL_ROTATION_BUTTON_3 = "small_rotation_switch_3"
string                      SMALL_ROTATION_BUTTON_4 = "small_rotation_switch_4"
string                      SMALL_ROTATION_BUTTON_5 = "small_rotation_switch_5"

string                      ELECTROMAGNET = "electromagnet"

# ----------------------------------------------------------------------------------------------

## Manipulate objects of the maintenance panel with SAM model
string                      ALIGN_OBJECT_WITH_SAM   = "align_object_with_sam"

string                      MODEL_BIG_ROTATION_BUTTON = "model_big_rotation_switch"
string                      MODEL_SMALL_ROTATION_BUTTON = "model_small_rotation_switch"

# ----------------------------------------------------------------------------------------------

## Small Autonomous Task for ERC 2025
string                      TURN_J6 = "turn_j6"

string                      TURN_JSIX_POS_3 = "turn_j6_30_pos"
string                      TURN_JSIX_POS_4 = "turn_j6_45_pos"
string                      TURN_JSIX_POS_9 = "turn_j6_90_pos"
string                      TURN_JSIX_NEG_3 = "turn_j6_30_neg"
string                      TURN_JSIX_NEG_4 = "turn_j6_45_neg"
string                      TURN_JSIX_NEG_9 = "turn_j6_90_neg"

string                      PROBE = "probe_deposit"

string                      PROBE_UP_1 = "probe_u_1"
string                      PROBE_UP_2 = "probe_u_2"
string                      PROBE_UP_3 = "probe_u_3"
string                      PROBE_SIDE_1 = "probe_s_1"
string                      PROBE_SIDE_2 = "probe_s_2"
string                      PROBE_SIDE_3 = "probe_s_3"


# ----------------------------------------------------------------------------------------------

## Big Autonomous Task for ERC 2025
string                      ROCK = "rocks"
string                      ARUCO_APPROACH = "aruco_approach"
string                      SAND = "sand_collect"
string                      QRCODE = "qr_code"

# ----------------------------------------------------------------------------------------------

## MESSAGE DATA
string                      target
string[]                    maintenance_objects
string                      model_element

int8                        rotation_degree
string                      clockwise_or_not

int8                        probe_number
string                      probe_orientation

string                      predefined_pose

# ERROR_CODES
string                      UNKNOWN_GOAL = "unknown goal"