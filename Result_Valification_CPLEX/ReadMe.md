CPLEX Batch Verifier

Overview
- This tool verifies solution CSVs produced by the solver against instance files.
- It now reads all CSV files placed in `Final_Results/` (or a specified results folder) and dynamically finds the `Item_0` column to extract solution vectors.
- For each input CSV a verification report is written to `Results_Verified/`.

How it works
- The verifier indexes all instance files under the provided instances directory (`--dir`). Keys are constructed as `Type_Size_Index` using the instance path structure.
- For each CSV in `Final_Results/` the program reads the header line to locate columns:
  - `Type`, `Size`, `Index` (required to build the Key)
  - optional `Seed` column (if present will be included in the report)
  - `BestFound`/`BestFoundObj` (used as InputObj)
  - `Time` / `Time(s)` (this time value from the input CSV is used in the report)
  - `Item_0`, `Item_1`, ... (located dynamically; items do not need to start at a fixed column)
- For each row the program fixes variables according to the extracted Item_* columns and runs CPLEX on the corresponding instance to verify feasibility and objective value.

Output
- For each input CSV file `X.csv` the verifier writes `Results_Verified/Verification_Report_X.csv`.
- If the input filename starts with `All_Best_Found_Results_`, that prefix is removed before adding `Verification_Report_`.
- Each verification report uses this column order:
  Key,Seed,Status,InputObj,CalculatedObj,IsFeasible,ObjMatch,Time(s),InstancePath

Usage
1. Build (requires CPLEX and a working CMake environment):

```bash
mkdir -p build && cd build
cmake ..
make -j
```

2. Run (default reads `Final_Results` in current folder):

```bash
# using default Final_Results folder
./build/CplexVerifier --dir ./Instances

# or specify a results folder or a single CSV file
./build/CplexVerifier --dir ./Instances --csv ./Final_Results
./build/CplexVerifier --dir ./Instances --csv ./Final_Results/somefile.csv
```

Notes and assumptions
- The program expects instance files arranged so that `PU::GetKeyFromPath(path)` produces `Type_Size_Index` (existing project behavior).
- `Time(s)` in the report is taken from the input CSV (the solver's reported time), not the verification runtime.
- If the input CSV does not include a `Seed` column the report will leave that field empty.
- Reports are placed under `Results_Verified/` (created if missing).

If you'd like, I can now run the verifier on the current `Final_Results` folder and produce the `Results_Verified` reports (this requires CPLEX and may take time).