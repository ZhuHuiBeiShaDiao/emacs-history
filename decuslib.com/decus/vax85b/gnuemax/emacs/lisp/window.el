;; GNU Emacs window commands aside from those written in C. 
;; Copyright (C) 1985 Richard M. Stallman. 
 
;; This file is part of GNU Emacs. 
 
;; GNU Emacs is distributed in the hope that it will be useful, 
;; but without any warranty.  No author or distributor 
;; accepts responsibility to anyone for the consequences of using it 
;; or for whether it serves any particular purpose or works at all, 
;; unless he says so in writing. 
 
;; Everyone is granted permission to copy, modify and redistribute 
;; GNU Emacs, but only under the conditions described in the 
;; document "GNU Emacs copying permission notice".   An exact copy 
;; of the document is supposed to have been given to you along with 
;; GNU Emacs so that you can know how you may redistribute it all. 
;; It should be in a file named COPYING.  Among other things, the 
;; copyright notice and this notice must be preserved on all copies. 
 
 
(defun split-window-vertically (arg) 
  "Split current window into two windows, one above the other. 
This window becomes the uppermost of the two, and gets 
ARG lines.  No arg means split equally." 
  (interactive "P") 
  (split-window nil (and arg (prefix-numeric-value arg)))) 
 
(defun split-window-horizontally (arg) 
  "Split current window into two windows side by side. 
This window becomes the leftmost of the two, and gets 
ARG columns.  No arg means split equally." 
  (interactive "P") 
  (split-window nil (and arg (prefix-numeric-value arg)) t)) 
 
(defun enlarge-window-horizontally (arg) 
  "Make current window ARG columns wider." 
  (interactive "p") 
  (enlarge-window arg t)) 
 
(defun shrink-window-horizontally (arg) 
  "Make current window ARG columns narrower." 
  (interactive "p") 
  (shrink-window arg t)) 
 
(define-key ctl-x-map "2" 'split-window-vertically) 
(define-key ctl-x-map "5" 'split-window-horizontally) 
(define-key ctl-x-map "}" 'enlarge-window-horizontally) 
(define-key ctl-x-map "{" 'shrink-window-horizontally) 
