#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "openarmx_hands_hig/joystick_protocol.hpp"

using openarmx_hands_hig::JoystickFrame;
using openarmx_hands_hig::button_values;
using openarmx_hands_hig::joystick_values;
using openarmx_hands_hig::parse_joystick_line;
using openarmx_hands_hig::normalize_axis;
using openarmx_hands_hig::public_accessory_values;
using openarmx_hands_hig::AxisEventState;
using openarmx_hands_hig::ButtonEventState;
using openarmx_hands_hig::should_publish_axes;
using openarmx_hands_hig::should_publish_button;
using openarmx_hands_hig::update_toggle_button;

TEST(JoystickProtocol, ParsesTwelveColumnsAndPreservesAccessoryValues)
{
  JoystickFrame frame;
  ASSERT_TRUE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t532\t481\t0\t1\t0", &frame));
  EXPECT_EQ(frame.hand_id, 1);
  EXPECT_EQ(frame.values, (std::vector<int32_t>{63, 42, 77, 57, 27, 0}));
  EXPECT_EQ(frame.accessories, (std::vector<int32_t>{532, 481, 0, 1, 0}));
}

TEST(JoystickProtocol, RejectsFramesWithUnexpectedColumnCount)
{
  JoystickFrame frame;
  EXPECT_FALSE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0", &frame));
  EXPECT_FALSE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t532\t481\t0\t1\t0\t9", &frame));
}

TEST(JoystickProtocol, RejectsNonNumericOrOutOfRangeHandValues)
{
  JoystickFrame frame;
  EXPECT_FALSE(parse_joystick_line("left\t63\t42\t77\t57\t27\t0\t532\t481\t0\t0\t0", &frame));
  EXPECT_FALSE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t532\t481\t0\t0\t999999999999", &frame));
}

TEST(JoystickProtocol, AcceptsObservedLeftAndRightHandIdentifiers)
{
  JoystickFrame left;
  JoystickFrame right;
  ASSERT_TRUE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t532\t481\t0\t0\t0", &left));
  ASSERT_TRUE(parse_joystick_line("2\t15\t57\t68\t41\t14\t42\t528\t501\t0\t0\t0", &right));
  EXPECT_EQ(left.hand_id, 1);
  EXPECT_EQ(right.hand_id, 2);
}

TEST(JoystickProtocol, SplitsAccessoryChannelsForJoystickAndButtons)
{
  JoystickFrame frame;
  ASSERT_TRUE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t532\t481\t0\t1\t0", &frame));
  EXPECT_EQ(joystick_values(frame), (std::vector<int32_t>{532, 481, 0}));
  EXPECT_EQ(button_values(frame), (std::vector<int32_t>{1, 0}));
}

TEST(JoystickProtocol, NormalizesAdcAxisWithFixedCenterAndRawDeadzone)
{
  EXPECT_DOUBLE_EQ(normalize_axis(510), 0.0);
  EXPECT_DOUBLE_EQ(normalize_axis(410), 0.0);
  EXPECT_DOUBLE_EQ(normalize_axis(610), 0.0);
  EXPECT_NEAR(normalize_axis(0), -1.0, 1e-9);
  EXPECT_NEAR(normalize_axis(1023), 1.0, 1e-9);
  EXPECT_NEAR(normalize_axis(816), 206.0 / 413.0, 1e-9);
  EXPECT_NEAR(normalize_axis(205), -0.5, 1e-9);
}

TEST(JoystickProtocol, NormalizesAdcAxisClampsOutOfRangeValues)
{
  EXPECT_DOUBLE_EQ(normalize_axis(-10), -1.0);
  EXPECT_DOUBLE_EQ(normalize_axis(1100), 1.0);
}

TEST(JoystickProtocol, AppliesConfiguredAxisSign)
{
  EXPECT_NEAR(normalize_axis(0, 510, 100, 0, 1023, true), 1.0, 1e-9);
  EXPECT_NEAR(normalize_axis(1023, 510, 100, 0, 1023, true), -1.0, 1e-9);
  EXPECT_NEAR(normalize_axis(0, 510, 100, 0, 1023, false), -1.0, 1e-9);
}

TEST(JoystickProtocol, OrdersPublicAccessoriesAsXYClickAndButtons)
{
  JoystickFrame frame;
  ASSERT_TRUE(parse_joystick_line("1\t63\t42\t77\t57\t27\t0\t100\t200\t1\t0\t1", &frame));
  EXPECT_EQ(public_accessory_values(frame, true), (std::vector<int32_t>{200, 100, 1, 0, 1}));
  EXPECT_EQ(public_accessory_values(frame, false), (std::vector<int32_t>{100, 200, 1, 1, 0}));
}

TEST(JoystickProtocol, PublishesButtonWhileHeldAndOnRelease)
{
  ButtonEventState state;
  EXPECT_FALSE(should_publish_button(false, &state));
  EXPECT_TRUE(should_publish_button(true, &state));
  EXPECT_TRUE(should_publish_button(true, &state));
  EXPECT_TRUE(should_publish_button(true, &state));
  EXPECT_TRUE(should_publish_button(false, &state));
  EXPECT_FALSE(should_publish_button(false, &state));
}

TEST(JoystickProtocol, PublishesAxesContinuouslyUntilOneCenterEvent)
{
  AxisEventState state;
  EXPECT_FALSE(should_publish_axes(0.0, 0.0, 0.05, &state));
  EXPECT_TRUE(should_publish_axes(0.5, 0.0, 0.05, &state));
  EXPECT_TRUE(should_publish_axes(0.6, 0.0, 0.05, &state));
  EXPECT_TRUE(should_publish_axes(0.0, 0.0, 0.05, &state));
  EXPECT_FALSE(should_publish_axes(0.0, 0.0, 0.05, &state));
}

TEST(JoystickProtocol, TogglesButtonOnlyOnPressRisingEdges)
{
  ButtonEventState state;
  EXPECT_FALSE(update_toggle_button(false, &state));
  EXPECT_TRUE(update_toggle_button(true, &state));
  EXPECT_TRUE(state.latched);
  EXPECT_FALSE(update_toggle_button(true, &state));
  EXPECT_FALSE(update_toggle_button(false, &state));
  EXPECT_TRUE(update_toggle_button(true, &state));
  EXPECT_FALSE(state.latched);
}
