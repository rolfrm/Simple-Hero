(entity :id "player"
	(circle 0.0 0.0 1.0))

(entity :id "enemy"
	:color (from.rgb 255 0 0)
	(circle 0.0 0.0 0.1))

(scenery :id "world"
  (add (circle 5 5 3)
       (sub (circle 0.0 0.0 10.0)
            (circle 0.0 0.0 8.0))))
