from pprint import pprint
from random import randrange, choice
from PIL import Image

def random_triple(x,y,z):
    return (randrange(x), randrange(y), randrange(z))

class Cube:
    def __init__(self, x,y,z):
        self.x, self.y, self.z = x ,y, z
        self.cube = [[[None for i in range(z)] for i in range(y)] for i in range(x)]
        self.coloured = [random_triple(x,y,z) for _ in range(40)]
        for (x,y,z) in self.coloured:
            self.cube[x][y][z] = random_triple(255,255,255)
        self.fill_cube()
    def get(self,pos): return self.cube[pos[0]][pos[1]][pos[2]]
    def set(self,pos, val): self.cube[pos[0]][pos[1]][pos[2]] = val
    def in_range(self, pos):
        (x, y, z) = pos
        return x >= 0 and x < self.x and \
                y >= 0 and y < self.y and \
                z >= 0 and z < self.z 
    def neighbours(self, pos):
        (x, y, z) = pos
        sides = range(-1, 2)
        return [(x+i,y+j,z+k) for i in sides for j in sides for k in sides
                if not (i == j == k == 0) and self.in_range((x+i, y+j, z+k))]
    def fill_cube(self):
        completed = 0
        while self.coloured != []:
            pos_idx = randrange(len(self.coloured))
            pos = self.coloured[pos_idx]
            ns = self.neighbours(pos)
            empties = [n for n in ns if self.get(n) is None]
            colours = [pos] + [n for n in ns if self.get(n) is not None]
            if empties == []: 
                del self.coloured[pos_idx]
                completed += 1
                if completed % 100 == 0:
                    print("%i / %i"%(completed, self.x * self.y * self.z))
            else:
                empty = choice(empties)
                ncolours = len(colours)
                red = max(0, 10 - randrange(20) + sum([self.get(c)[0] for c in colours]) / ncolours)
                gre = max(0, 10 - randrange(20) + sum([self.get(c)[1] for c in colours]) / ncolours)
                blu = max(0, 10 - randrange(20) + sum([self.get(c)[2] for c in colours]) / ncolours)
                self.set(empty, (int(red),int(gre),int(blu)))
                self.coloured.append(empty)
    def to_gif(self, filename):
        for z in range(self.z):
            img = Image.new( 'RGB', (self.x,self.y), "black") # create a new black image
            pixels = img.load() # create the pixel map

            for i in range(self.x):
                for j in range(self.y):
                    pixels[i,j] = self.get((i,j,z)) # set the colour accordingly
            img.save("test%05d.jpg"%(z), "JPEG")

    def test(self):
        self.to_gif("")

if __name__ == "__main__":
    Cube(150,150,150).test()
