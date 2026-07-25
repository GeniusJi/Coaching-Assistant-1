import pandas as pd

# Read the original CSV
df = pd.read_csv("data.csv")

# Remove goalkeepers
df = df[df["Position"] != "GKP"]

# Columns to keep
columns_to_keep = [
    "Player Name",
    "Position",
    "Minutes",
    "Touches",
    "Shots",
    "Progressive Carries",
    "Interceptions",
    "fThird Passes",
    "Successful fThird Passes",
    "Crosses",
    "Successful Crosses",
    "Ground Duels",
    "gDuels Won"
]

# Keep only the selected columns
filtered_df = df[columns_to_keep]

# Save to a new CSV
filtered_df.to_csv("filtered_data.csv", index=False)

print(f"Successfully created filtered_data.csv with {len(filtered_df)} outfield players.")