//
//  main.cpp
//  cube
//
//  Created by Vilhelm Hedberg on 12/14/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>


// Functional utilities

template<typename... Args>
bool all(Args... args) { return (... && args); }

// Axes:
// X -> Right, Y -> Up, Z -> Forward
enum class Axis : uint8_t {
    X,
    Y,
    Z
};

// Face order:
// Top, Left, Front, Right, Back, Bottom

enum class Color : uint8_t {
    White = 0,
    Orange,
    Green,
    Red,
    Blue,
    Yellow,
    None
};

constexpr const char* to_string(const Color& c)
{
    switch (c) {
        case Color::White:
            return "W";
        case Color::Orange:
            return "O";
        case Color::Green:
            return "G";
        case Color::Red:
            return "R";
        case Color::Blue:
            return "B";
        case Color::Yellow:
            return "Y";
        case Color::None:
            return "-";
    }
}

enum class Face : uint8_t {
    Top = 0,
    Left,
    Front,
    Right,
    Back,
    Bottom
};

struct Layer {
    constexpr Layer(Axis a, size_t i) : axis(a), index(i) {}
    Axis axis;
    size_t index;
};

constexpr Layer layer_left(const size_t cubeSize) { return Layer(Axis::X, 0); }
constexpr Layer layer_middle(const size_t cubeSize) { return Layer(Axis::X, cubeSize / 2); }
constexpr Layer layer_right(const size_t cubeSize) { return Layer(Axis::X, cubeSize - 1); }
constexpr Layer layer_bottom(const size_t cubeSize) { return Layer(Axis::Y, 0); }
constexpr Layer layer_equator(const size_t cubeSize) { return Layer(Axis::Y, cubeSize / 2); }
constexpr Layer layer_top(const size_t cubeSize) { return Layer(Axis::Y, cubeSize - 1); }
constexpr Layer layer_front(const size_t cubeSize) { return Layer(Axis::Z, 0); }
constexpr Layer layer_slice(const size_t cubeSize) { return Layer(Axis::Z, cubeSize / 2); }
constexpr Layer layer_back(const size_t cubeSize) { return Layer(Axis::Z, cubeSize - 1); }

constexpr Layer layer(Face face, const size_t cubeSize) {
    switch (face) {
        case Face::Top:
            return layer_top(cubeSize);
        case Face::Left:
            return layer_left(cubeSize);
        case Face::Front:
            return layer_front(cubeSize);
        case Face::Right:
            return layer_right(cubeSize);
        case Face::Back:
            return layer_back(cubeSize);
        case Face::Bottom:
            return layer_bottom(cubeSize);
    }
}

constexpr Color default_color(Face f)
{
    switch (f) {
        case Face::Top:
            return Color::White;
        case Face::Left:
            return Color::Orange;
        case Face::Front:
            return Color::Green;
        case Face::Right:
            return Color::Red;
        case Face::Back:
            return Color::Blue;
        case Face::Bottom:
            return Color::Yellow;
    }
}

typedef std::array<Color, 6> Piece;

namespace piece {
    
    Piece default_piece()
    {
        Piece p;
        p.fill(Color::None);
        return p;
    }
    
    Piece default_face(Face f)
    {
        Piece p = default_piece();
        p[static_cast<size_t>(f)] = default_color(f);
        return p;
    }
    
    Piece with_color(Face f, Color c)
    {
        Piece p = default_piece();
        p[static_cast<size_t>(f)] = c;
        return p;
    }
    
    Piece add_default_face(Piece&& p, Face f)
    {
        p[static_cast<size_t>(f)] = default_color(f);
        return p;
    }
    
    Piece add_color(Piece&& p, Face f, Color c)
    {
        p[static_cast<size_t>(f)] = c;
        return p;
    }
    
    constexpr bool has(const Piece& p, Face f)
    {
        return p[static_cast<size_t>(f)] != Color::None;
    }
    
    Piece shift(Piece&& p, std::array<Face, 4>&& faceIndices, bool clockwise)
    {
        Color prev;
        if (!clockwise) {
            for (auto it = faceIndices.begin(); it != faceIndices.end(); it++) {
                std::swap(p[static_cast<size_t>(*it)], prev);
            }
            std::swap(p[static_cast<size_t>(faceIndices.front())], prev);
        } else {
            for (auto it = faceIndices.rbegin(); it != faceIndices.rend(); it++) {
                std::swap(p[static_cast<size_t>(*it)], prev);
            }
            std::swap(p[static_cast<size_t>(faceIndices.back())], prev);
        }
        return p;
    }
    
    Piece rotate(Piece&&p, Axis axis, bool clockwise)
    {
        switch (axis) {
            case Axis::X:
                return shift(std::move(p), {Face::Top, Face::Back, Face::Bottom, Face::Front}, clockwise);
            case Axis::Y:
                return shift(std::move(p), {Face::Front, Face::Left, Face::Back, Face::Right}, clockwise);
            case Axis::Z:
                return shift(std::move(p), {Face::Top, Face::Left, Face::Bottom, Face::Right}, clockwise);
        }
    }
    
    void print(const Piece& p, std::ostream& os)
    {
        os << "  " << to_string(p[0]) << "\n";
        for (size_t i = 1; i < 5; i++) {
            os << to_string(p[i]) << " ";
        }
        os << "\n";
        os << "  " << to_string(p[5]) << "\n";
    }
    
    std::ostream& print_face(const Piece& p, Face face, std::ostream& os)
    {
        return os << to_string(p[static_cast<size_t>(face)]);
    }
}

template <size_t Size> using Cube = std::array<Piece, Size * Size * Size>;
typedef Cube<3> Cube3;


namespace cube {
    
    struct Coord
    {
        size_t x, y, z;
    };
    
    template<size_t Size>
    constexpr size_t index(size_t x, size_t y, size_t z)
    {
        return (z * Size + y) * Size + x;
    }
    
    template<size_t Size>
    constexpr size_t index(const Coord& c)
    {
        return index<Size>(c.x, c.y, c.z);
    }
    
    typedef std::function<Coord(size_t, size_t)> PlaneToCubeMapping;
    
    template<size_t Size>
    PlaneToCubeMapping planeToCube(Layer l)
    {
        auto planeToCube = [](size_t begin, int xStride, int yStride) -> PlaneToCubeMapping {
            return [begin, xStride, yStride](size_t lx, size_t ly) -> Coord {
                size_t idx = begin + (ly*yStride) + lx*xStride;
                size_t z = idx / (Size * Size);
                size_t layerIdx = idx - z * Size * Size;
                size_t y = layerIdx / Size;
                size_t x = layerIdx % Size;
                return {.x = x, .y = y, .z = z};
            };
        };
        
        constexpr auto isFirstHalf = [](size_t index) -> bool { return index <= Size / 2; };
        constexpr auto inverseIndex = [](size_t index) -> size_t { return Size - 1 - index; };
        
        switch (l.axis) {
            case Axis::X:
                return planeToCube(isFirstHalf(l.index) ? index<Size>(l.index, 0, 0) : index<Size>(Size - 1 - inverseIndex(l.index), Size - 1, 0),
                                   (isFirstHalf(l.index) ? 1 : -1) * static_cast<int>(Size),
                                   Size * Size);
            case Axis::Y:
                return planeToCube(isFirstHalf(l.index) ? index<Size>(0, Size - 1, Size - 1 - l.index) : index<Size>(0, 0, Size - 1 - l.index),
                                   1,
                                   (isFirstHalf(l.index) ? -1 : 1) * static_cast<int>(Size));
            case Axis::Z:
                return planeToCube(isFirstHalf(l.index) ? index<Size>(0, Size - 1 - l.index, 0) : index<Size>(Size - 1, Size - 1 - l.index, 0),
                                   isFirstHalf(l.index) ? 1 : -1,
                                   Size * Size);
        }
    }
    
    template<size_t Size>
    Cube<Size> default_cube()
    {
        typedef typename Cube<Size>::iterator CubeIterator;
        auto createRow = [](CubeIterator rowBegin, bool hasTopOrBottom, Face topOrBottom, bool hasFrontOrBack, Face frontOrBack) {
            CubeIterator rowIt = rowBegin;
            
            // Left
            *rowIt = piece::default_face(Face::Left);
            if (hasFrontOrBack) {
                *rowIt = piece::add_default_face(std::move(*rowIt), frontOrBack);
            }
            rowIt++;
            
            // Middle
            for (size_t i = 1; i < Size - 1; i++, rowIt++) {
                if (hasFrontOrBack) {
                    *rowIt = piece::add_default_face(std::move(*rowIt), frontOrBack);
                }
            }
            
            // Right
            *rowIt = piece::default_face(Face::Right);
            if (hasFrontOrBack) {
                *rowIt = piece::add_default_face(std::move(*rowIt), frontOrBack);
            }
            

            if (hasTopOrBottom) {
                rowIt = rowBegin;
                for (size_t i = 0; i < Size; i++, rowIt++) {
                    *rowIt = piece::add_default_face(std::move(*rowIt), topOrBottom);
                }
            }
        };
        
        auto rowIterator = [](Cube<Size>& c, size_t y, size_t z) -> CubeIterator {
            return c.begin() + index<Size>(0, y, z);
        };
        
        Cube<Size> cube;
        cube.fill(piece::default_piece());
        
        // Top layer
        size_t z = 0;
        createRow(rowIterator(cube, 0, z), true, Face::Top, true, Face::Back);
        for (size_t y = 1; y < Size - 1; y++) {
            createRow(rowIterator(cube, y, z), true, Face::Top, false, Face::Back);
        }
        createRow(rowIterator(cube, Size - 1, z), true, Face::Top, true, Face::Front);
        
        // Middle layers
        for (z = 1; z < Size - 1; z++) {
            createRow(rowIterator(cube, 0, z), false, Face::Top, true, Face::Back);
            for (size_t y = 1; y < Size; y++) {
                createRow(rowIterator(cube, y, z), false, Face::Top, false, Face::Back);
            }
            createRow(rowIterator(cube, Size - 1, z), false, Face::Top, true, Face::Front);
        }

        // Bottom layer
        z = Size - 1;
        createRow(rowIterator(cube, 0, z), true, Face::Bottom, true, Face::Back);
        for (size_t y = 1; y < Size; y++) {
            createRow(rowIterator(cube, y, z), true, Face::Bottom, false, Face::Back);
        }
        createRow(rowIterator(cube, Size - 1, z), true, Face::Bottom, true, Face::Front);
        
        return cube;
    }
    
    template<size_t Size>
    bool validate(const Cube<Size>& cube)
    {
        auto validateSide = [&cube](PlaneToCubeMapping plane2Cube, Face face) -> bool {
            for (size_t y = 0; y < Size; y++) {
                for (size_t x = 0; x < Size; x++) {
                    const Piece& p = cube[index<Size>(plane2Cube(x, y))];
                    if (!piece::has(p, face)) {
                        return false;
                    }
                }
            }
            return true;
        };
        
        return all(validateSide(planeToCube<Size>(layer(Face::Top, Size)), Face::Top),
                   validateSide(planeToCube<Size>(layer(Face::Left, Size)), Face::Left),
                   validateSide(planeToCube<Size>(layer(Face::Front, Size)), Face::Front),
                   validateSide(planeToCube<Size>(layer(Face::Right, Size)), Face::Right),
                   validateSide(planeToCube<Size>(layer(Face::Back, Size)), Face::Back),
                   validateSide(planeToCube<Size>(layer(Face::Bottom, Size)), Face::Bottom));
    }
    
    template<size_t Size>
    Cube<Size> rotate(Cube<Size>&& cube, Layer layer, bool clockwise)
    {
        // 1. First rotate the layer
        PlaneToCubeMapping mapping = planeToCube<Size>(layer);
        //   1.a First flip
        if (clockwise) {
            // Flip vertically
            for (size_t y = 0; y < Size/2; y++) {
                for (size_t x = 0; x < Size; x++) {
                    std::swap(cube[index<Size>(mapping(x, y))], cube[index<Size>(mapping(x, Size - 1 - y))]);
                }
            }
        } else {
            // Flip horizontally
            for (size_t y = 0; y < Size; y++) {
                for (size_t x = 0; x < Size/2; x++) {
                    std::swap(cube[index<Size>(mapping(x, y))], cube[index<Size>(mapping(Size - 1 - x, y))]);
                }
            }
        }
        
        //   1.b Transpose
        for (size_t y = 0; y < Size - 1; y++) {
            for (size_t x = y + 1; x < Size; x++) {
                std::swap(cube[index<Size>(mapping(x, y))], cube[index<Size>(mapping(y, x))]);
            }
        }
        
        // 2. Then rotate all the pieces in the layer
        const bool clockwisePiece = (layer.index <= Size/2);
        for (size_t y = 0; y < Size; y++) {
            for (size_t x = 0; x < Size; x++) {
                cube[index<Size>(mapping(x, y))] = piece::rotate(std::move(cube[index<Size>(mapping(x, y))]), layer.axis, clockwisePiece ? clockwise : !clockwise);
            }
        }
        
        return cube;
    }

    template<size_t Size>
    std::ostream& print(const Cube<Size>& cube, std::ostream& os)
    {
        auto printRow = [](const Cube<Size>& cube, std::ostream& os, size_t spaces, size_t row, Face face) -> std::ostream& {
            PlaneToCubeMapping mapping = planeToCube<Size>(layer(face, Size));
            for (size_t s = 0; s < spaces; s++) os << " ";
            for (size_t x = 0; x < Size; x++) {
                const Piece& p = cube[index<Size>(mapping(x, row))];
                piece::print_face(p, face, os) << " ";
            }
            return os;
        };
        
        for (size_t y = 0; y < Size; y++) {
            printRow(cube, os, Size*2, y, Face::Top) << "\n";
        }
        for (size_t y = 0; y < Size; y++) {
            printRow(cube, os, 0, y, Face::Left);
            printRow(cube, os, 0, y, Face::Front);
            printRow(cube, os, 0, y, Face::Right);
            printRow(cube, os, 0, y, Face::Back);
            os << "\n";
        }
        for (size_t y = 0; y < Size; y++) {
            printRow(cube, os, Size*2, y, Face::Bottom) << "\n";
        }
        return os;
    }
}

int main(int argc, const char * argv[]) {
    
    Piece p = piece::add_color(piece::add_color(piece::with_color(Face::Top, Color::White), Face::Front, Color::Green), Face::Left, Color::Orange);
    piece::print(p, std::cout);
    p = piece::rotate(std::move(p), Axis::X, true);
    p = piece::rotate(std::move(p), Axis::X, true);
    piece::print(p, std::cout);
    
    constexpr size_t kCubeSize = 3;
    Cube<kCubeSize> c = cube::default_cube<kCubeSize>();
    int a = 2;
    
    cube::PlaneToCubeMapping tm = cube::planeToCube<kCubeSize>(layer_equator(kCubeSize));
    for (size_t y = 0; y < kCubeSize; y++) {
        for (size_t x = 0; x < kCubeSize; x++) {
            cube::Coord c = tm(x, y);
            printf("(%d, %d) -> (%d, %d, %d)\n", x, y, c.x, c.y, c.z);
        }
    }
    
    std::cout << "Valid: " << cube::validate<kCubeSize>(c) << "\n";
    
    c = cube::rotate<kCubeSize>(std::move(c), layer_back(kCubeSize), false);
    
    cube::print<kCubeSize>(c, std::cout);
    
    std::cout << "Valid: " << cube::validate<kCubeSize>(c) << "\n";
    
    return 0;
}
