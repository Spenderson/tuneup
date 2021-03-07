

# display a status: when to get maintenance and what to get

### user input

"Enter the current mileage of the car: "

### create a table, one obs per task

```
var curMileage = [user_input];
var modelYear = 2016;
var curDate = [date_global_variable];
var daysUntil _daysUntil;
var milesUntil _milesUntil;
const BIRTHDAY = dateToInteger("01JAN" || toString(modelYear));

%macro forLoop1(days)
  for each object in "tasks" {
    daysUntil = BIRTHDAY + &days - curDate;
    for each object in "history" {
      _daysUntil = date + &days - curDate;
      if (_daysUntil > daysUntil) then daysUntil = _daysUntil;
    }
  }
%mend;

%macro forLoop2(miles, days)
  for each object in "tasks" {
    milesUntil = &miles - curMileage;
    daysUntil = BIRTHDAY + &days - curDate;
    for each object in "history" {
      _milesUntil = mileage + &miles - curMileage;
      _daysUntil = date + &days - curDate;
      if (_milesUntil > milesUntil) then milesUntil = _milesUntil;
      if (_daysUntil  > daysUntil)  then daysUntil  = _daysUntil;
    }
  }
%mend;

%macro forLoop3(milesFirst, daysFirst, milesAfter, daysAfter);
  for each object in "tasks" {
    milesUntil = &milesFirst - curMileage;
    daysUntil = BIRTHDAY + &daysFirst - curDate;
    for each object in "history" {
      _milesUntil = mileage + &milesAfter - curMileage;
      _daysUntil = date + &daysAfter - curDate;
      if (_milesUntil > milesUntil) then milesUntil = _milesUntil;
      if (_daysUntil  > daysUntil)  then daysUntil  = _daysUntil;
    }
  }
%mend;

for each object in log.json {
  if (frequency == "twice a year") {
    %forLoop1(182)
  } else if (frequency == "every 7,500 miles (12,000 km) or 6 months") {
    %forLoop2(7500, 182)
  } else if (frequency == "once a year") {
    %forLoop1(365)
  } else if (frequency == "every 15,000 miles (24,000 km) or 12 months") {
    %forLoop2(15000, 365)
  } else if (frequency == "every 30,000 miles (48,000 km) or 24 months") {
    %forLoop2(30000, 730)
  } else if (frequency == "every 37,500 miles (60,000 km) or 30 months") {
    %forLoop2(37500, 912)
  } else if (frequency == "first at 60,000 miles (96,000 km) or 72 months; After, every 15,000 miles (24,000 km) or 24 months") {
    %forLoop3(60000, 2190, 15000, 730)
  } else if (frequency == "first at 60,000 miles (96,000 km) or 60 months; After, every 30,000 miles (48,000 km) or 24 months") {
    %forLoop3(60000, 1825, 30000, 730)
  } else if (frequency == "every 105,000 miles (168,000 km) or 84 months") {
    %forLoop2(105000, 2555)
  } else [throw warning: "Unrecognized FREQUENCY: " frequency=]
}
```

### sort and output the table

In the table, create variable:
  ```
  var overdue = FALSE;
  if (daysUntil < 1) or (milesUntil < 1) then overdue = TRUE;
  ```

sort new table by: (descending) `overdue`, `daysUntil`, `milesUntil`, `task`

output the table like this:
    
| **Overdue** |
| --- |
| Task 3 (54 days, 308 mi) |
| Task 1 (32 days) |
| Task 4 (2 days) |
| Task 2 (1204 mi) |

| **Coming up** |
| --- |
| Task 11 (4 days, 475 mi) |
| Task 9  (6 days, 9328 mi) |
| Task 5  (23 days, 4 mi) |
| Task 7  (35 days, 98 mi) |
| Task 6  (42 days, 56 mi) |
| Task 8  (43 days, 536 mi) |
| Task 10 (45 days, 3246 mi) |
| Task 12 (56 days, 102  mi) |
