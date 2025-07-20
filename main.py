
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
        scale: Vector2 | tuple[float, float] = (16, 16),
        texture_ids: list[(int, Vector2)] = [(0, Vector2(0, 0))]
    ):
        self.id = len(Mob.instances)
        Mob.instances.append(self)

        self.position: Vector2
        self.scale: Vector2

        self.speed: float = 1200
        self.friction: float = 1

        self.direction: Vector2 = Vector2(1, 1)
        self.last_position: Vector2

        self.texture_ids: list[(int, Vector2)] = texture_ids
        self.current_texture_id: int = 0
        self.current_texture_timer: float = 0
        self.texture_start_time: float = 0.18

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

        self.direction.x = direction.x if direction.x else self.direction.x
        self.direction.y = direction.y if direction.y else self.direction.y
        direction = direction.normalize()

        if direction.length() > 0:
            acceleration = acceleration + direction * self.speed

            if self.current_texture_timer == 0:
                self.current_texture_id += 1
                if self.current_texture_id > 1:
                    self.current_texture_id = 0
                self.current_texture_timer = self.texture_start_time

        elif velocity.length() > 0:
            acceleration = acceleration - velocity * self.friction

        self.last_position = self.position
        self.position = self.position + \
            (velocity + acceleration * dt) * dt

        if self.current_texture_timer > 0:
            self.current_texture_timer -= dt
            if self.current_texture_timer <= 0:
                self.current_texture_timer = 0

    def draw(self):
        pyx.blt(
            self.position.x,
            self.position.y,
            self.texture_ids[self.current_texture_id][0],
            self.scale.x * self.texture_ids[self.current_texture_id][1].x,
            self.scale.y * self.texture_ids[self.current_texture_id][1].y,
            -self.direction.x * self.scale.x,
            self.scale.y,
            0
        )


class World:
    def __init__(self):
        self.player = Mob((10, 10), texture_ids=[
            (0, Vector2(1, 0)), (0, Vector2(2, 0)), (0, Vector2(3, 0))
        ])
        # self.map =


world = World()


class App:
    def __init__(self):
        pyx.init(160, 120, title="Hello World!")
        pyx.load("my_resource.pyxres")

        # pyx.colors[1] = 0x111111

        self.last_time = time.time()
        self.hud_visible: bool = True
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

        if pyx.btnp(pyx.KEY_SPACE):
            self.hud_visible = not self.hud_visible
        world.player.update(dt, direction)

    def draw(self):
        pyx.cls(0)
        # pyx.text(55, 41, "Hello World!", 5)

        world.player.draw()
        if self.hud_visible:
            pyx.text(
                world.player.position.x,
                world.player.position.y - 5,
                "liam",
                1
            )

App()
