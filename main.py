import cv2
import numpy as np
import heapq
import matplotlib.pyplot as plt
import time
import serial

# Global variables to store start and goal
start = None
goal = None
clicks = []  # Store clicked points

def image_to_binary_array(image_path, threshold=128):
    image = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    if image is None:
        raise ValueError("Image not found or could not be loaded.")
    _, binary_image = cv2.threshold(image, threshold, 1, cv2.THRESH_BINARY_INV)
    return binary_image

def heuristic(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1])

def a_star_search(grid, start, goal):
    rows, cols = grid.shape
    open_set = []
    heapq.heappush(open_set, (0, start))
    came_from = {}
    g_score = {start: 0}
    f_score = {start: heuristic(start, goal)}
    while open_set:
        _, current = heapq.heappop(open_set)
        if current == goal:
            path = []
            while current in came_from:
                path.append(current)
                current = came_from[current]
            path.append(start)
            path.reverse()
            return path
        neighbors = [(0,1), (1,0), (0,-1), (-1,0)]
        for dx, dy in neighbors:
            neighbor = (current[0] + dx, current[1] + dy)
            if 0 <= neighbor[0] < rows and 0 <= neighbor[1] < cols and grid[neighbor] == 0:
                tentative_g_score = g_score[current] + 1
                if neighbor not in g_score or tentative_g_score < g_score[neighbor]:
                    came_from[neighbor] = current
                    g_score[neighbor] = tentative_g_score
                    f_score[neighbor] = tentative_g_score + heuristic(neighbor, goal)
                    heapq.heappush(open_set, (f_score[neighbor], neighbor))
    return []

def draw_path(grid, path):
    plt.ion()
    plt.imshow(grid, cmap='gray')
    for click in clicks:
        plt.scatter(click[1], click[0], c='red', s=50)  # Ensure dots remain visible
    for i in range(len(path) - 1):
        plt.plot([path[i][1], path[i+1][1]], [path[i][0], path[i+1][0]], c='red', linewidth=2)
        plt.pause(0.0000001)  # Faster animation speed
    plt.ioff()
    plt.show()

def on_click(event):
    global start, goal, clicks
    if event.xdata is None or event.ydata is None:
        return
    x, y = int(event.ydata), int(event.xdata)
    if binary_array[x, y] == 0:  # Only allow selection on valid paths
        clicks.append((x, y))  # Store click location
        plt.scatter(y, x, c='red', s=50)  # Place red dot at click location
        plt.draw()
        if start is None:
            start = (x, y)
            print(f"Start selected at: {start}")
        elif goal is None:
            goal = (x, y)
            print(f"Goal selected at: {goal}")
            plt.close()  # Close figure after selecting goal

def select_start_goal(grid):
    global start, goal, binary_array, clicks
    binary_array = grid
    clicks = []  # Reset clicks
    fig, ax = plt.subplots()
    ax.imshow(grid, cmap='gray')
    fig.canvas.mpl_connect('button_press_event', on_click)
    plt.show()

def get_directions(path):
    directions = []
    for i in range(len(path) - 1):
        current = path[i]
        next_pos = path[i + 1]
        dx = next_pos[1] - current[1]  # x coordinate (column)
        dy = next_pos[0] - current[0]  # y coordinate (row)
        
        if dx == 1:
            directions.append('right')
        elif dx == -1:
            directions.append('left')
        elif dy == 1:
            directions.append('down')
        elif dy == -1:
            directions.append('up')
    
    return directions

def main():
    ser = serial.Serial('/dev/ttyACM0', 9600)
    time.sleep(2)

    image_path = "restaurant.png"
    binary_array = image_to_binary_array(image_path)
    select_start_goal(binary_array)
    if start and goal:
        path = a_star_search(binary_array, start, goal)
        if path:
            print("Path found:", path)
            directions = get_directions(path)
            print("Directions:", directions)
            draw_path(binary_array, path)
            for dir in directions:
                ser.write((dir+"\n").encode('utf-8'))
                print(f"Sent: {dir}")
                time.sleep(5)
            ser.close();
        else:
            print("No path found.")

if __name__ == "__main__":
    main()