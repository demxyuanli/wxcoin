#pragma once

enum class ShapeId {
    None, Main, Edge, Corner, Button
};

enum class PickId {
    None,
    Front,
    Top,
    Right,
    Rear,
    Bottom,
    Left,
    FrontTop,
    FrontBottom,
    FrontRight,
    FrontLeft,
    RearTop,
    RearBottom,
    RearRight,
    RearLeft,
    TopRight,
    TopLeft,
    BottomRight,
    BottomLeft,
    FrontTopRight,
    FrontTopLeft,
    FrontBottomRight,
    FrontBottomLeft,
    RearTopRight,
    RearTopLeft,
    RearBottomRight,
    RearBottomLeft,
    ArrowNorth,
    ArrowSouth,
    ArrowEast,
    ArrowWest,
    ArrowLeft,
    ArrowRight,
    DotBackside,
    ViewMenu,
    ArrowUp,
    ArrowDown
};
