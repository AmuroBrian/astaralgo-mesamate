import cv2
import numpy as np
import heapq
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk
from tkinter import messagebox
import time
import serial
import threading
from PIL import Image, ImageTk

# Define stations and their coordinates
STATIONS = {
    'table1': (43, 146),
    'table2': (43, 355),
    'table3': (255, 355),
    'table4': (255, 146)
}

class MesamateApp:
    def __init__(self, root):
        self.root = root
        self.root.title("MESAMATE")
        self.root.geometry("800x480")  # Reduced height for 10.1-inch display
        
        # Set theme colors
        self.theme_color = "#fffa82"
        self.accent_color = "#4a4a4a"
        self.text_color = "#2c2c2c"
        
        # Initialize serial communication
        try:
            # Try different possible serial ports for Raspberry Pi
            possible_ports = ['/dev/ttyACM0', '/dev/ttyUSB0', '/dev/ttyAMA0']
            self.serial_port = None
            
            for port in possible_ports:
                try:
                    print(f"Attempting to connect to {port}...")
                    self.serial_port = serial.Serial(port, 9600, timeout=1)
                    # Wait for Arduino to reset
                    time.sleep(2)
                    print(f"Successfully connected to {port}")
                    break
                except Exception as e:
                    print(f"Failed to connect to {port}: {str(e)}")
                    continue
                    
            if self.serial_port is None:
                raise Exception("No valid serial port found")
                
            self.serial_thread = threading.Thread(target=self.serial_listener, daemon=True)
            self.serial_thread.start()
            
            # Test communication
            self.serial_port.write(b"test\n")
            time.sleep(0.1)
            if self.serial_port.in_waiting:
                response = self.serial_port.readline().decode().strip()
                print(f"Arduino response: {response}")
                
        except Exception as e:
            print(f"Error initializing serial port: {e}")
            messagebox.showerror("Serial Error", 
                               "Failed to initialize serial communication.\n" +
                               "Please check if Arduino is connected and try again.")
            self.serial_port = None
            
        # Configure root window
        self.root.configure(bg=self.theme_color)
        
        # Make root window responsive
        self.root.grid_rowconfigure(0, weight=1)
        self.root.grid_columnconfigure(0, weight=1)
        
        # Set protocol for window close
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # Initialize variables
        self.selected_tables = []
        self.binary_array = None
        self.fig = None
        self.ax = None
        self.canvas = None
        self.current_path_index = 0
        self.paths_to_process = []
        self.processing_path = False
        
        # Create welcome screen
        self.create_welcome_screen()
        
    def serial_listener(self):
        while True:
            if self.serial_port and self.serial_port.is_open:
                try:
                    if self.serial_port.in_waiting:
                        response = self.serial_port.readline().decode().strip()
                        print(f"Received from Arduino: {response}")
                        if response == "DIRECTION_DONE":
                            # Only process next direction after receiving DIRECTION_DONE
                            self.root.after(100, self.process_next_direction)  # Added small delay
                except Exception as e:
                    print(f"Error reading from serial port: {e}")
            time.sleep(0.1)
            
    def send_direction_to_arduino(self, direction):
        if self.serial_port and self.serial_port.is_open:
            try:
                # Send single direction
                direction_str = direction + "\n"
                self.serial_port.write(direction_str.encode())
                print(f"Sent to Arduino: {direction_str.strip()}")
                # Flush to ensure the data is sent immediately
                self.serial_port.flush()
                # Wait for acknowledgment
                time.sleep(0.1)
            except Exception as e:
                print(f"Error sending to Arduino: {e}")
                messagebox.showerror("Communication Error", "Failed to send direction to Arduino")
                
    def process_next_direction(self):
        try:
            if not self.processing_path and self.current_path_index < len(self.paths_to_process):
                current_path = self.paths_to_process[self.current_path_index]
                
                if not hasattr(self, 'current_direction_index'):
                    self.current_direction_index = 0
                
                if self.current_direction_index < len(current_path['directions']):
                    # Get current direction
                    current_direction = current_path['directions'][self.current_direction_index]
                    print(f"\nProcessing Path {self.current_path_index + 1}: {current_path['description']}")
                    print(f"Current direction {self.current_direction_index + 1}/{len(current_path['directions'])}: {current_direction}")
                    
                    # Send direction to Arduino
                    self.send_direction_to_arduino(current_direction)
                    
                    # Clear the previous path and redraw the base image
                    self.ax.clear()
                    self.ax.imshow(self.binary_array, cmap='gray')
                    
                    # Redraw all station points and labels
                    for station_name, coords in STATIONS.items():
                        self.ax.scatter(coords[1], coords[0], c='blue', s=100)
                        self.ax.text(coords[1], coords[0] - 10, f"Table {station_name[-1]}", 
                                    ha='center', va='bottom', color='blue', fontsize=10)
                    
                    # Redraw initial position
                    initial_position = (0, self.binary_array.shape[1] // 2)
                    self.ax.scatter(initial_position[1], initial_position[0], c='green', s=100)
                    self.ax.text(initial_position[1], initial_position[0] - 10, "Initial Position",
                                ha='center', va='bottom', color='green', fontsize=10)
                    
                    # Draw the current path up to this direction
                    path = current_path['path']
                    path_x = [p[1] for p in path]
                    path_y = [p[0] for p in path]
                    self.ax.plot(path_x, path_y, c='red', linewidth=2)
                    self.canvas.draw()
                    
                    # Move to next direction
                    self.current_direction_index += 1
                    
                else:
                    # All directions for current path completed
                    path_number = self.current_path_index + 1
                    print(f"\nPath {path_number} completed: {current_path['description']}")
                    
                    # Show food delivery confirmation for current table
                    if self.current_path_index < len(self.selected_tables):
                        current_table = self.selected_tables[self.current_path_index]
                        # Wait for a short delay to ensure the path is visually completed
                        self.root.after(2000, lambda: self.confirm_delivery(current_table))
                    else:
                        print("Warning: No table selected for current path")
                        self.current_path_index += 1
                        self.current_direction_index = 0
                        if self.current_path_index < len(self.paths_to_process):
                            self.process_next_direction()
                    
        except tk.TclError:
            print("Window was destroyed during path processing")
            return
        except Exception as e:
            print(f"Error processing next direction: {e}")
            return

    def create_welcome_screen(self):
        # Clear any existing widgets
        for widget in self.root.winfo_children():
            widget.destroy()
            
        # Make window full screen (compatible with both macOS and Raspberry Pi OS)
        try:
            self.root.attributes('-fullscreen', True)  # For macOS
        except:
            try:
                self.root.state('zoomed')  # For Raspberry Pi OS
            except:
                self.root.geometry(f"{self.root.winfo_screenwidth()}x{self.root.winfo_screenheight()}+0+0")
        
        # Create main frame
        main_frame = tk.Frame(self.root, bg=self.theme_color)
        main_frame.grid(row=0, column=0, sticky="nsew")
        main_frame.grid_rowconfigure(0, weight=1)
        main_frame.grid_columnconfigure(0, weight=1)
        
        # Create a canvas for proper layering
        canvas = tk.Canvas(main_frame, highlightthickness=0)
        canvas.grid(row=0, column=0, sticky="nsew")
        main_frame.grid_rowconfigure(0, weight=1)
        main_frame.grid_columnconfigure(0, weight=1)
        
        # Create background image
        try:
            # Use PIL for better image handling
            bg_img = Image.open("mesamatebg.png")
            # Resize background image to fit screen
            screen_width = self.root.winfo_screenwidth()
            screen_height = self.root.winfo_screenheight()
            bg_img = bg_img.resize((screen_width, screen_height), Image.Resampling.LANCZOS)
            bg_image = ImageTk.PhotoImage(bg_img)
            # Add background to canvas
            canvas.create_image(0, 0, image=bg_image, anchor="nw")
            canvas.image = bg_image  # Keep a reference to prevent garbage collection
        except Exception as e:
            print(f"Error loading background image: {e}")
            # Fallback to solid color background
            canvas.configure(bg=self.theme_color)
            
        # Create logo image
        try:
            # Use PIL for better image handling
            logo_img = Image.open("mesamatelogo.png")
            # Calculate new size (reduce by 70% for smaller display)
            new_width = int(logo_img.width * 0.3)
            new_height = int(logo_img.height * 0.3)
            logo_img = logo_img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            logo_image = ImageTk.PhotoImage(logo_img)
            # Add logo to canvas
            canvas.create_image(
                screen_width // 2,
                screen_height * 0.35,  # Moved up slightly
                image=logo_image,
                anchor="center"
            )
            canvas.logo_image = logo_image  # Keep a reference to prevent garbage collection
        except Exception as e:
            print(f"Error loading logo image: {e}")
            # Fallback to text logo
            canvas.create_text(
                screen_width // 2,
                screen_height * 0.35,  # Moved up slightly
                text="MESAMATE",
                font=("Helvetica", 36, "bold"),  # Reduced font size
                fill="black",
                anchor="center"
            )
            
        # Create click instruction label with bold and italic
        canvas.create_text(
            screen_width // 2,
            screen_height * 0.85,  # Moved up to ensure visibility
            text="Click Anywhere to Begin",
            font=("Helvetica", 14, "bold italic"),  # Reduced font size
            fill="black",
            anchor="center"
        )
        
        # Bind click event
        self.root.bind("<Button-1>", self.show_table_selection)
        
        # Bind Escape key to exit full screen
        def exit_fullscreen(event):
            try:
                self.root.attributes('-fullscreen', False)  # For macOS
            except:
                try:
                    self.root.state('normal')  # For Raspberry Pi OS
                except:
                    pass
        self.root.bind("<Escape>", exit_fullscreen)
        
    def create_rounded_button(self, parent, text, command, width=15, height=1, font_size=10, is_bold=False):  # Reduced sizes
        # Create a frame for the button
        button_frame = tk.Frame(
            parent,
            bg=self.theme_color,
            padx=1,
            pady=1
        )
        
        # Create the actual button
        btn = tk.Button(
            button_frame,
            text=text,
            width=width,
            height=height,
            font=("Helvetica", font_size, "bold" if is_bold else "normal"),
            bg="white",
            fg="black",
            activebackground="#f0f0f0",
            activeforeground="black",
            relief=tk.FLAT,
            command=command,
            borderwidth=0,
            highlightthickness=0
        )
        btn.pack(fill="both", expand=True)
        
        # Add hover effect
        def on_enter(e):
            btn.configure(bg="#f0f0f0")
            
        def on_leave(e):
            btn.configure(bg="white")
            
        btn.bind("<Enter>", on_enter)
        btn.bind("<Leave>", on_leave)
        
        return button_frame

    def show_table_selection(self, event):
        # Clear the selected tables array when entering selection screen
        self.selected_tables = []
        
        # Create a new window for table selection
        self.selection_window = tk.Toplevel(self.root)
        self.selection_window.title("Select Tables")
        self.selection_window.geometry("400x480")
        self.selection_window.configure(bg=self.theme_color)
        
        # Center the window
        self.center_window(self.selection_window)
        
        # Make window responsive
        self.selection_window.grid_rowconfigure(0, weight=1)
        self.selection_window.grid_columnconfigure(0, weight=1)
        
        # Create main container
        main_container = tk.Frame(self.selection_window, bg=self.theme_color)
        main_container.grid(row=0, column=0, sticky="nsew", padx=10, pady=10)
        
        # Create title label
        title_label = tk.Label(
            main_container,
            text="Select Customer's Table",
            font=("Helvetica", 20, "bold"),
            bg=self.theme_color,
            fg=self.text_color
        )
        title_label.pack(pady=10)
        
        # Create frame for buttons
        button_frame = tk.Frame(main_container, bg=self.theme_color)
        button_frame.pack(pady=10, fill="x")
        
        # Create table buttons with custom style
        self.table_buttons = {}
        for table in ['table1', 'table2', 'table3', 'table4']:
            btn_frame = self.create_rounded_button(
                button_frame,
                f"Table {table[-1]}",
                lambda t=table: self.select_table(t)
            )
            self.table_buttons[table] = btn_frame
            btn_frame.pack(pady=10)
        
        # Create control buttons frame
        control_frame = tk.Frame(main_container, bg=self.theme_color)
        control_frame.pack(pady=20, fill="x")
        
        # Test LEDs button
        test_leds_btn_frame = self.create_rounded_button(
            control_frame,
            "Test LEDs",
            self.test_leds,
            width=15,
            height=2,
            font_size=12,
            is_bold=True
        )
        test_leds_btn_frame.pack(side=tk.LEFT, padx=10, expand=True)
        
        # Start button
        start_btn_frame = self.create_rounded_button(
            control_frame,
            "START",
            self.start_path_visualization,
            width=15,
            height=2,
            font_size=12,
            is_bold=True
        )
        start_btn_frame.pack(side=tk.LEFT, padx=10, expand=True)
        
        # Clear button
        clear_btn_frame = self.create_rounded_button(
            control_frame,
            "CLEAR",
            self.clear_selection,
            width=15,
            height=2,
            font_size=12,
            is_bold=True
        )
        clear_btn_frame.pack(side=tk.LEFT, padx=10, expand=True)
        
        # Create frame for selected tables display
        selected_frame = tk.Frame(main_container, bg=self.theme_color)
        selected_frame.pack(pady=20, fill="x")
        
        # Label for selected tables
        selected_label = tk.Label(
            selected_frame,
            text="Tables Selected:",
            font=("Helvetica", 14, "bold"),
            bg=self.theme_color,
            fg=self.text_color
        )
        selected_label.pack(pady=(0, 10))
        
        # Frame for table boxes
        self.table_boxes_frame = tk.Frame(selected_frame, bg=self.theme_color)
        self.table_boxes_frame.pack(fill="x")
        
        # Initialize the table boxes display
        self.update_table_boxes()
        
        # Reset all button states to default
        self.reset_button_states()
        
    def center_window(self, window):
        # Update window to get correct dimensions
        window.update_idletasks()
        
        # Get screen width and height
        screen_width = window.winfo_screenwidth()
        screen_height = window.winfo_screenheight()
        
        # Get window width and height
        window_width = window.winfo_width()
        window_height = window.winfo_height()
        
        # Calculate position
        x = (screen_width - window_width) // 2
        y = (screen_height - window_height) // 2
        
        # Set window position
        window.geometry(f"+{x}+{y}")
        
        # Ensure window stays on top
        window.lift()
        window.attributes('-topmost', True)
        window.after_idle(window.attributes, '-topmost', False)
        
    def select_table(self, table):
        # Check if table is already selected
        if table in self.selected_tables:
            # Remove the table if it's already selected (toggle off)
            self.selected_tables.remove(table)
            # Update button state to white
            self.update_button_state(table, False)
        else:
            # Check maximum table limit
            if len(self.selected_tables) >= 3:
                messagebox.showwarning(
                    "Maximum Tables Reached",
                    "You can only select up to 3 tables at a time.\nPlease clear your selection or remove a table first."
                )
                return
            # Add the table to selection
            self.selected_tables.append(table)
            # Update button state to selected
            self.update_button_state(table, True)
            
        # Update visual feedback
        self.update_table_boxes()
        
    def update_button_state(self, table, is_selected):
        # Find and update the specific button
        for widget in self.selection_window.winfo_children():
            if isinstance(widget, tk.Frame):
                for child in widget.winfo_children():
                    if isinstance(child, tk.Frame):
                        for button in child.winfo_children():
                            if isinstance(button, tk.Button):
                                button_text = button.cget("text")
                                if f"Table {table[-1]}" == button_text:
                                    if is_selected:
                                        button.configure(
                                            bg="#e0e0e0",  # Light gray for selected
                                            relief=tk.SUNKEN
                                        )
                                    else:
                                        button.configure(
                                            bg="white",  # White for unselected
                                            relief=tk.FLAT
                                        )
                                    
    def update_button_states(self):
        # Get all table buttons
        for widget in self.selection_window.winfo_children():
            if isinstance(widget, tk.Frame):
                for child in widget.winfo_children():
                    if isinstance(child, tk.Frame):
                        for button in child.winfo_children():
                            if isinstance(button, tk.Button):
                                table_num = button.cget("text").split()[-1]
                                table_name = f"table{table_num}"
                                
                                # Update button appearance based on selection state
                                if table_name in self.selected_tables:
                                    button.configure(
                                        bg="#e0e0e0",  # Light gray for selected
                                        relief=tk.SUNKEN
                                    )
                                else:
                                    button.configure(
                                        bg="white",  # White for unselected
                                        relief=tk.FLAT
                                    )
                                
                                # Disable button if max tables reached and not selected
                                if len(self.selected_tables) >= 3 and table_name not in self.selected_tables:
                                    button.configure(state=tk.DISABLED)
                                else:
                                    button.configure(state=tk.NORMAL)
                                    
    def reset_button_states(self):
        # Reset all table buttons to default state
        for table, btn_frame in self.table_buttons.items():
            for widget in btn_frame.winfo_children():
                if isinstance(widget, tk.Button):
                    widget.configure(
                        bg="white",
                        relief=tk.FLAT,
                        state=tk.NORMAL
                    )
                    
    def clear_selection(self):
        # Clear the selected tables array
        self.selected_tables = []
        
        # Update visual feedback
        self.update_table_boxes()
        
        # Reset all button states to default
        self.reset_button_states()
        
    def update_table_boxes(self):
        # Clear existing table boxes
        for widget in self.table_boxes_frame.winfo_children():
            widget.destroy()
            
        if not self.selected_tables:
            # Show "No tables selected" in a box
            no_tables_frame = tk.Frame(
                self.table_boxes_frame,
                bg="white",
                padx=20,
                pady=10
            )
            no_tables_frame.pack(pady=5)
            
            no_tables_label = tk.Label(
                no_tables_frame,
                text="No tables selected",
                font=("Helvetica", 12),
                bg="white",
                fg=self.accent_color
            )
            no_tables_label.pack()
        else:
            # Create a frame for horizontal alignment
            boxes_container = tk.Frame(self.table_boxes_frame, bg=self.theme_color)
            boxes_container.pack(pady=5)
            
            # Create boxes for each selected table
            for i, table in enumerate(self.selected_tables):
                table_frame = tk.Frame(
                    boxes_container,
                    bg="white",
                    padx=15,
                    pady=8
                )
                table_frame.pack(side=tk.LEFT, padx=5)
                
                table_label = tk.Label(
                    table_frame,
                    text=f"Table {table[-1]}",
                    font=("Helvetica", 12, "bold"),
                    bg="white",
                    fg="black"
                )
                table_label.pack()
                
                # Add arrow between tables (except for the last one)
                if i < len(self.selected_tables) - 1:
                    arrow_label = tk.Label(
                        boxes_container,
                        text="→",
                        font=("Helvetica", 16, "bold"),
                        bg=self.theme_color,
                        fg=self.text_color
                    )
                    arrow_label.pack(side=tk.LEFT, padx=5)
        
    def start_path_visualization(self):
        # Validate selection before starting
        if not self.selected_tables:
            messagebox.showwarning(
                "No Tables Selected",
                "Please select at least one table before starting."
            )
            return
            
        if len(self.selected_tables) > 3:
            messagebox.showerror(
                "Invalid Selection",
                "Maximum 3 tables allowed. Please clear and reselect."
            )
            return
            
        # Reset all LEDs before starting new path
        if self.serial_port and self.serial_port.is_open:
            try:
                # Turn off all LEDs first
                command = "PATH_START:4\n"  # This will turn on all LEDs
                self.serial_port.write(command.encode())
                self.serial_port.flush()
                time.sleep(0.5)
                
                # Then turn off all LEDs
                for path in range(1, 4):
                    command = f"FOOD_RECEIVED:{path}\n"
                    self.serial_port.write(command.encode())
                    self.serial_port.flush()
                    time.sleep(0.1)
                
                print("Reset all LEDs before starting new path")
            except Exception as e:
                print(f"Error resetting LEDs: {e}")
            
        # Close selection window
        self.selection_window.destroy()
        
        # Clear main window
        for widget in self.root.winfo_children():
            widget.destroy()
            
        # Load and process the image
        try:
            image_path = "demolayout.png"
            self.binary_array = self.image_to_binary_array(image_path)
            
            # Create matplotlib figure with custom style
            plt.style.use('default')  # Using default style instead of seaborn
            self.fig, self.ax = plt.subplots(figsize=(10, 8))
            self.fig.patch.set_facecolor(self.theme_color)
            self.ax.imshow(self.binary_array, cmap='gray')
            
            # Create canvas
            self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)
            self.canvas.draw()
            self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            
            # Process the path
            self.process_station_sequence(self.binary_array, self.selected_tables)
            
        except Exception as e:
            messagebox.showerror(
                "Error",
                f"An error occurred while processing the path: {str(e)}\nPlease try again."
            )
            # Reset the application state
            self.selected_tables = []
            self.create_welcome_screen()
            
    def image_to_binary_array(self, image_path, threshold=128):
        image = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
        if image is None:
            raise ValueError("Image not found or could not be loaded.")
        _, binary_image = cv2.threshold(image, threshold, 1, cv2.THRESH_BINARY_INV)
        return binary_image
        
    def heuristic(self, a, b):
        return abs(a[0] - b[0]) + abs(a[1] - b[1])
        
    def a_star_search(self, grid, start, goal):
        rows, cols = grid.shape
        open_set = []
        heapq.heappush(open_set, (0, start))
        came_from = {}
        g_score = {start: 0}
        f_score = {start: self.heuristic(start, goal)}
        
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
                        f_score[neighbor] = tentative_g_score + self.heuristic(neighbor, goal)
                        heapq.heappush(open_set, (f_score[neighbor], neighbor))
        return []
        
    def get_directions(self, path):
        directions = []
        current_dir = None
        count = 0
        
        for i in range(len(path) - 1):
            current = path[i]
            next_pos = path[i + 1]
            dx = next_pos[1] - current[1]  # x coordinate (column)
            dy = next_pos[0] - current[0]  # y coordinate (row)
            
            # Determine direction
            if dx == 1:
                new_dir = 'right'
            elif dx == -1:
                new_dir = 'left'
            elif dy == 1:
                new_dir = 'down'
            elif dy == -1:
                new_dir = 'up'
                
            # Count consecutive same directions
            if new_dir == current_dir:
                count += 1
            else:
                if current_dir is not None:
                    directions.append(f"{count}{current_dir}")
                current_dir = new_dir
                count = 1
        
        # Add the last direction
        if current_dir is not None:
            directions.append(f"{count}{current_dir}")
        
        return directions

    def process_station_sequence(self, grid, stations):
        rows, cols = grid.shape
        initial_position = (0, cols // 2)
        self.paths_to_process = []  # Reset paths list
        self.current_path_index = 0
        self.processing_path = False
        
        print("\n=== Starting Path Processing ===")
        print(f"Selected tables: {stations}")
        
        # Draw all station points with labels
        for station_name, coords in STATIONS.items():
            self.ax.scatter(coords[1], coords[0], c='blue', s=100)
            self.ax.text(coords[1], coords[0] - 10, f"Table {station_name[-1]}", 
                        ha='center', va='bottom', color='blue', fontsize=10)
        
        # Add label for initial position
        self.ax.scatter(initial_position[1], initial_position[0], c='green', s=100)
        self.ax.text(initial_position[1], initial_position[0] - 10, "Initial Position",
                    ha='center', va='bottom', color='green', fontsize=10)
        
        # First path: initial position to first station
        first_station = stations[0]
        print(f"\nCalculating Path 1: Initial Position to {first_station}")
        path = self.a_star_search(grid, initial_position, STATIONS[first_station])
        if path:
            directions = self.get_directions(path)
            self.paths_to_process.append({
                'path': path,
                'directions': directions,
                'description': f"Path 1 (Initial to {first_station})"
            })
            print(f"Path 1 directions: {directions}")
        else:
            print(f"ERROR: No path found for Initial Position to {first_station}")
        
        # Process paths between consecutive stations
        for i in range(len(stations) - 1):
            current_station = stations[i]
            next_station = stations[i + 1]
            print(f"\nCalculating Path {i+2}: {current_station} to {next_station}")
            path = self.a_star_search(grid, STATIONS[current_station], STATIONS[next_station])
            if path:
                directions = self.get_directions(path)
                self.paths_to_process.append({
                    'path': path,
                    'directions': directions,
                    'description': f"Path {i+2} ({current_station} to {next_station})"
                })
                print(f"Path {i+2} directions: {directions}")
            else:
                print(f"ERROR: No path found for {current_station} to {next_station}")
        
        # Final path: last station back to initial position
        last_station = stations[-1]
        print(f"\nCalculating Final Path: {last_station} to Initial Position")
        path = self.a_star_search(grid, STATIONS[last_station], initial_position)
        if path:
            directions = self.get_directions(path)
            self.paths_to_process.append({
                'path': path,
                'directions': directions,
                'description': f"Path {len(stations)+1} ({last_station} to Initial)"
            })
            print(f"Final path directions: {directions}")
        else:
            print(f"ERROR: No path found for {last_station} to Initial Position")
        
        print("\n=== All Paths Calculated ===")
        print(f"Total paths to process: {len(self.paths_to_process)}")
        
        # Start processing the first path
        self.process_next_direction()
        
    def show_completion_message(self):
        try:
            # Check if root window still exists
            try:
                if not self.root.winfo_exists():
                    return
            except tk.TclError:
                print("Window was destroyed before completion message could be shown")
                return
                
            # Create completion message frame
            completion_frame = tk.Frame(self.root, bg=self.theme_color)
            completion_frame.pack(pady=10)
            
            # Completion message
            completion_label = tk.Label(
                completion_frame,
                text="ALL ORDERS HAVE BEEN DELIVERED",
                font=("Helvetica", 16, "bold"),
                bg=self.theme_color,
                fg=self.text_color
            )
            completion_label.pack(pady=(0, 10))
            
            # Create frame for delivery confirmations
            delivery_frame = tk.Frame(completion_frame, bg=self.theme_color)
            delivery_frame.pack(pady=10)
            
            # Add delivery confirmation buttons for each table
            for table in self.selected_tables:
                table_frame = tk.Frame(delivery_frame, bg=self.theme_color)
                table_frame.pack(pady=5)
                
                # Table label
                table_label = tk.Label(
                    table_frame,
                    text=f"Table {table[-1]}",
                    font=("Helvetica", 12, "bold"),
                    bg=self.theme_color,
                    fg=self.text_color
                )
                table_label.pack(side=tk.LEFT, padx=10)
                
                # Confirm delivery button
                confirm_btn = self.create_rounded_button(
                    table_frame,
                    "Confirm Delivery",
                    lambda t=table: self.confirm_delivery(t),
                    width=15,
                    height=1,
                    font_size=10
                )
                confirm_btn.pack(side=tk.LEFT, padx=10)
            
            # OKAY button
            okay_btn_frame = self.create_rounded_button(
                completion_frame,
                "OKAY",
                self.reset_and_return_to_welcome,
                width=15,
                height=2,
                font_size=14,
                is_bold=True
            )
            okay_btn_frame.pack(pady=10)
            
            # Show completion popup
            self.show_completion_popup()
        except tk.TclError:
            print("Window was destroyed before completion message could be shown")
            return
        except Exception as e:
            print(f"Error showing completion message: {e}")
            return

    def show_completion_popup(self):
        # Create a new window for completion message
        popup = tk.Toplevel(self.root)
        popup.title("Order Completion")
        popup.geometry("400x200")
        popup.configure(bg=self.theme_color)
        
        # Make window modal
        popup.transient(self.root)
        popup.grab_set()
        
        # Center the window
        self.center_window(popup)
        
        # Create main container
        main_container = tk.Frame(popup, bg=self.theme_color)
        main_container.pack(pady=20, padx=20, fill="both", expand=True)
        
        # Title
        title_label = tk.Label(
            main_container,
            text="All Orders Have Been Completed!",
            font=("Helvetica", 16, "bold"),
            bg=self.theme_color,
            fg=self.text_color
        )
        title_label.pack(pady=(0, 20))
        
        # Message
        message_label = tk.Label(
            main_container,
            text="The robot has successfully delivered all orders.",
            font=("Helvetica", 12),
            bg=self.theme_color,
            fg=self.text_color,
            wraplength=350
        )
        message_label.pack(pady=10)
        
        # OKAY button
        okay_btn = self.create_rounded_button(
            main_container,
            "OKAY",
            lambda: [popup.destroy(), self.show_table_selection(None)],
            width=15,
            height=2,
            font_size=14,
            is_bold=True
        )
        okay_btn.pack(pady=20)
        
        # Wait for window to be closed
        self.root.wait_window(popup)

    def confirm_delivery(self, table):
        try:
            # Check if root window still exists
            try:
                if not self.root.winfo_exists():
                    return
            except tk.TclError:
                print("Window was destroyed before showing confirmation")
                return

            # Create a new window for food delivery confirmation
            confirm_window = tk.Toplevel(self.root)
            confirm_window.title("Food Delivery Confirmation")
            confirm_window.geometry("400x300")
            confirm_window.configure(bg=self.theme_color)
            
            # Make window modal
            confirm_window.transient(self.root)
            confirm_window.grab_set()
            
            # Center the window
            self.center_window(confirm_window)
            
            # Create main container
            main_container = tk.Frame(confirm_window, bg=self.theme_color)
            main_container.pack(pady=20, padx=20, fill="both", expand=True)
            
            # Title
            title_label = tk.Label(
                main_container,
                text="Food Delivery Confirmation",
                font=("Helvetica", 16, "bold"),
                bg=self.theme_color,
                fg=self.text_color
            )
            title_label.pack(pady=(0, 20))
            
            # Message
            message_label = tk.Label(
                main_container,
                text=f"Has the food been received by the customer at Table {table[-1]}?",
                font=("Helvetica", 12),
                bg=self.theme_color,
                fg=self.text_color,
                wraplength=350
            )
            message_label.pack(pady=10)
            
            # Button frame
            button_frame = tk.Frame(main_container, bg=self.theme_color)
            button_frame.pack(pady=20)
            
            # Yes button
            yes_btn = self.create_rounded_button(
                button_frame,
                "Yes, Received",
                lambda: self.handle_food_received(table, confirm_window),
                width=15,
                height=1,
                font_size=12
            )
            yes_btn.pack(side=tk.LEFT, padx=10)
            
            # No button
            no_btn = self.create_rounded_button(
                button_frame,
                "No, Not Yet",
                lambda: self.handle_not_received(confirm_window),
                width=15,
                height=1,
                font_size=12
            )
            no_btn.pack(side=tk.LEFT, padx=10)
            
            # Wait for window to be closed
            self.root.wait_window(confirm_window)
                
        except tk.TclError:
            print("Window was destroyed during confirmation")
            return
        except Exception as e:
            print(f"Error showing food delivery confirmation: {e}")
            return

    def handle_food_received(self, table, window):
        try:
            # Show success message
            messagebox.showinfo(
                "Delivery Confirmed",
                f"Food delivery for Table {table[-1]} has been confirmed.\nThank you for using MESAMATE!"
            )
            
            # Close the confirmation window
            window.destroy()
            
            # Move to next path
            self.current_path_index += 1
            self.current_direction_index = 0
            
            if self.current_path_index >= len(self.paths_to_process):
                # All paths processed, show completion message
                print("\nAll orders have been completed!")
                try:
                    if self.root.winfo_exists():
                        self.show_completion_message()
                except tk.TclError:
                    print("Window was destroyed during path completion")
                    return
            else:
                # Process next path
                print(f"\nMoving to next path: {self.current_path_index + 1}")
                self.root.after(1000, self.process_next_direction)  # Add delay before starting next path
                
        except Exception as e:
            print(f"Error in handle_food_received: {e}")
            # Ensure window is destroyed even if there's an error
            try:
                window.destroy()
            except:
                pass

    def handle_not_received(self, window):
        # Send command to Arduino to activate buzzer
        if self.serial_port and self.serial_port.is_open:
            try:
                command = "BUZZER_ON\n"
                print("Sending buzzer activation command")
                self.serial_port.write(command.encode())
                self.serial_port.flush()
                print("Sent buzzer activation command")
            except Exception as e:
                print(f"Error sending buzzer command: {e}")
        
        # Close the confirmation window
        window.destroy()

    def reset_and_return_to_welcome(self):
        # Clear the selected tables array
        self.selected_tables = []
        
        # Clear any existing path visualization
        if hasattr(self, 'canvas'):
            self.canvas.get_tk_widget().destroy()
        if hasattr(self, 'fig'):
            plt.close(self.fig)
            
        # Return to welcome screen
        self.create_welcome_screen()

    def on_closing(self):
        # Close serial port if open
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            
        # Clear any existing selections
        self.selected_tables = []
        
        # Close matplotlib figure if it exists
        if hasattr(self, 'fig'):
            plt.close(self.fig)
            
        # Destroy the root window
        self.root.destroy()
        
        # Exit the program
        exit()

    def test_leds(self):
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.write(b"TEST_LEDS\n")
                self.serial_port.flush()
                print("Sent LED test command")
            except Exception as e:
                print(f"Error sending LED test command: {e}")
                messagebox.showerror("Error", "Failed to send LED test command")

def main():
    root = tk.Tk()
    app = MesamateApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()