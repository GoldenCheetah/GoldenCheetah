
import goldencheetah
import os

def test_read_ride():
    """
    Test reading a ride file using GoldenCheetah API.
    This script is intended to be run from the GoldenCheetah embedded Python console.
    """
    # Assuming we are running from the source root or test/python directory
    ride_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '../rides/test.json'))
    
    # Check if we can access the Bindings
    # Note: 'goldencheetah' module methods are often exposed directly or via 'gc' object in older setups,
    # but the SIP file defines a module 'goldencheetah'.
    
    print(f"GoldenCheetah version: {goldencheetah.version()}")
    
    # We don't have a direct 'loadRide' function exposed in Bindings.h seen so far,
    # but we can try investigating available attributes.
    print("Attributes in goldencheetah module:", dir(goldencheetah))

if __name__ == "__main__":
    test_read_ride()
