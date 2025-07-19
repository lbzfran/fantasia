
import pyxel as pyx
import time
import math


class Vector2:
    def __init__(self, x: int | float, y: int | float):
        self.x: int | float = x
        self.y: int | float = y

    def magnitude(self):
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def length(self):
        return self.magnitude()

    def normalize(self):
        mag = self.magnitude()
        if mag == 0:
            return Vector2(0, 0)
        return Vector2(self.x / mag, self.y / mag)

    def dot(self, other):
        if isinstance(other, Vector2):
            return self.x * other.x + self.y * other.y
        raise TypeError("Dot product requires another Vector2")

    def cross(self, other):
        if isinstance(other, Vector2):
            return self.x * other.y - self.y * other.x
        raise TypeError("Cross product requires another Vector2")

    def __str__(self):
        return f"({self.x}, {self.y})"

    def __eq__(self, other):
        return isinstance(other, Vector2) and self.x == other.x and self.y == other.y

    def __neg__(self):
        return Vector2(-self.x, -self.y)

    def __abs__(self):
        return Vector2(abs(self.x), abs(self.y))

    def __add__(self, other):
        if isinstance(other, Vector2):
            return Vector2(self.x + other.x, self.y + other.y)
        elif isinstance(other, (int, float)):
            return Vector2(self.x + other, self.y + other)
        raise TypeError(
            "Unsupported operand type(s) for +: 'Vector2' and '{}'".format(type(other).__name__))

    def __sub__(self, other):
        if isinstance(other, Vector2):
            return Vector2(self.x - other.x, self.y - other.y)
        elif isinstance(other, (int, float)):
            return Vector2(self.x - other, self.y - other)
        raise TypeError(
            "Unsupported operand type(s) for -: 'Vector2' and '{}'".format(type(other).__name__))

    def __mul__(self, other):
        if isinstance(other, Vector2):
            return self.dot(other)
        elif isinstance(other, (int, float)):
            return Vector2(self.x * other, self.y * other)
        raise TypeError(
            "Unsupported operand type(s) for *: 'Vector2' and '{}'".format(type(other).__name__))

    def __rmul__(self, other):
        return self * other


class Mob:
    instances = []

    def __init__(
        self,
        position: Vector2 | tuple[int | float, int | float] = (0, 0),
        scale: Vector2 | tuple[float, float] = (16, 16)
    ):
        self.id = len(Mob.instances)
        Mob.instances.append(self)

        self.position: Vector2
        self.scale: Vector2

        self.last_position: Vector2

        if isinstance(position, tuple):
            self.position = Vector2(position[0], position[1])
        elif isinstance(position, Vector2):
            self.position = position
        self.last_position = self.position

        if isinstance(scale, tuple):
            self.scale = Vector2(scale[0], scale[1])
        elif isinstance(scale, Vector2):
            self.scale = scale

    def update(self, dt, direction: Vector2):
        velocity: Vector2 = self.position - self.last_position
        acceleration: Vector2 = Vector2(0, 0)
        speed: float = 1000
        friction: float = 0.5

        if self.position.x < 0.0:
            self.position.x = 0
            self.last_position.x = self.position.x + velocity.x
        elif self.position.x > pyx.width - self.scale.x:
            self.position.x = pyx.width - self.scale.x
            self.last_position.x = self.position.x + velocity.x
        if self.position.y < 0.0:
            self.position.y = 0
            self.last_position.y = self.position.y + velocity.y
        elif self.position.y > pyx.height - self.scale.y:
            self.position.y = pyx.height - self.scale.y
            self.last_position.y = self.position.y + velocity.y

        if pyx.btn(pyx.KEY_W):
            direction.y -= 1
        if pyx.btn(pyx.KEY_S):
            direction.y += 1
        if pyx.btn(pyx.KEY_A):
            direction.x -= 1
        if pyx.btn(pyx.KEY_D):
            direction.x += 1
        direction = direction.normalize()

        if direction.length() > 0:
            acceleration = acceleration + direction * speed
        elif velocity.length() > 0:
            acceleration = acceleration - velocity * friction

        self.last_position = self.position
        self.position = self.position + \
            (velocity + acceleration * dt) * dt


class World:
    def __init__(self):
        self.player = Mob((10, 10))


world = World()


class App:
    def __init__(self):
        pyx.init(160, 120, title="Hello World!")
        pyx.load("my_resource.pyxres")

        # pyx.colors[1] = 0x111111

        self.last_time = time.time()
        pyx.run(self.update, self.draw)

    def update(self):
        now = time.time()
        dt = now - self.last_time
        self.last_time = now

        if pyx.btnp(pyx.KEY_Q):
            pyx.quit()

        direction: Vector2 = Vector2(0, 0)
        if pyx.btn(pyx.KEY_W):
            direction.y -= 1
        if pyx.btn(pyx.KEY_S):
            direction.y += 1
        if pyx.btn(pyx.KEY_A):
            direction.x -= 1
        if pyx.btn(pyx.KEY_D):
            direction.x += 1
        world.player.update(dt, direction)

    def draw(self):
        pyx.cls(0)
        # pyx.text(55, 41, "Hello World!", 5)

        # pyx.pset(world.player.position.x, world.player.position.y, 1)
        # pyx.rect(
        #     world.player.position.x,
        #     world.player.position.y,
        #     world.player.scale.x,
        #     world.player.scale.y,
        #     1
        # )

        pyx.blt(
            world.player.position.x,
            world.player.position.y,
            0,
            0,
            0,
            world.player.scale.x,
            world.player.scale.y,
            0
        )


App()
