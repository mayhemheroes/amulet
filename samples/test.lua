local am    = require "amulet"
local math  = require "math"
local coroutine  = require "coroutine"

local vshader = [[
    attribute float x;
    attribute float y;
    uniform mat4 MVP;
    void main() {
        gl_Position = MVP * vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    uniform vec4 tint;
    void main() {
        vec2 uv = gl_FragCoord.xy;
        gl_FragColor = vec4( 1, uv.x / 640.0, uv.y / 480.0, 1.0 ) * tint;
    }
]]

local win = am.create_window("hello", 640, 480)

local prog = am.program(vshader, fshader)

local buf = am.buffer(4 * 6)
local xview = buf:view("float", 0, 8)
local yview = buf:view("float", 4, 8)
xview[1] = -0.5
xview[2] = 0
xview[3] = 0.5
yview[1] = -0.4
yview[2] = 0.6
yview[3] = -0.4

local MVP1 = math.mat4(0.5)
MVP1:set(4, 4, 1)
local MVP2 = math.mat4(1)
MVP2:set(4, 1, -0.5)
local base = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)
local node1 = base:bind_mat4("MVP", MVP1):bind_vec4("tint", math.vec4(1, 1.5, 0.5, 1))
local node2 = base:bind_mat4("MVP", MVP2):bind_vec4("tint", math.vec4(0.1, 0.1, 1, 1))

node2.data.hello = "there"
print(node2.data.hello)

local group = am.empty()
group:append(node1)
group:append(node2)

local top = group:program(prog)

win.root = top
