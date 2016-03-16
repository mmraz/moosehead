/***************************************************************************
  *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
  *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
  *                                                                         *
  *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
  *  Chastain, Michael Quan, and Mitchell Tse.                              *
  *                                                                         *
  *  In order to use any part of this Merc Diku Mud, you must comply with   *
  *  both the original Diku license in 'license.doc' as well the Merc       *
  *  license in 'license.txt'.  In particular, you may not remove either of *
  *  these copyright notices.                                               *
  *                                                                         *
  *  Much time and thought has gone into this software and you are          *
  *  benefitting.  We hope that you share your changes too.  What goes      *
  *  around, comes around.                                                  *
  ***************************************************************************/
 
static char rcsid[] = "$Id: act_info.c,v 1.423 2004/04/02 04:42:07 boogums Exp $";
 #if defined(macintosh)
 #include <types.h>
 #else
 #include <sys/types.h>
 #include <unistd.h>
 #include <sys/time.h>
 #endif
 #include "gc.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <ctype.h>
 #include <time.h>
 #include "merc.h"
 #include "magic.h"
 #include "recycle.h"
 #include "tables.h"
 #include "lookup.h"
 #include "gladiator.h"
 
 /* command procedures needed */
 DECLARE_DO_FUN( do_exits        );
 DECLARE_DO_FUN( do_look         );
 DECLARE_DO_FUN( do_help         );
 DECLARE_DO_FUN( do_affects      );
 DECLARE_DO_FUN( do_play         );
 DECLARE_DO_FUN( do_outfit       );
 DECLARE_DO_FUN( do_dismount     ); 
 
 char *  const  where_name      [] =
 {
     "<{Wused as light{x>     ",
     "<{Wworn on finger{x>    ",
     "<{Wworn on finger{x>    ",
     "<{Wworn around neck{x>  ",
     "<{Wworn around neck{x>  ",
     "<{Wworn on torso{x>     ",
     "<{Wworn on head{x>      ",
     "<{Wworn on legs{x>      ",
     "<{Wworn on feet{x>      ",
     "<{Wworn on hands{x>     ",
     "<{Wworn on arms{x>      ",
     "<{Wworn as shield{x>    ",
     "<{Wworn about body{x>   ",
     "<{Wworn about waist{x>  ",
     "<{Wworn around wrist{x> ",
     "<{Wworn around wrist{x> ",
     "<{Wwielded{x>           ",
     "<{Wheld{x>              ",
     "<{Wfloating nearby{x>   ",
     "<{Wsecondary weapon{x>  "
}; 
 
 
 /* for do_count */
 int max_on = 0;
 
 
/*
  * Local functions.
  */
 char *  format_obj_to_char      args( ( OBJ_DATA *obj, CHAR_DATA *ch,
             bool fShort ) );
 void    show_char_to_char_0     args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
 void    show_char_to_char_1     args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
 void    show_char_to_char       args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
 bool    check_blind             args( ( CHAR_DATA *ch ) );
 bool    check_match             args( ( CHAR_DATA *ch, CHAR_DATA *victim) );


/** Ranger stuff **/
void do_species( CHAR_DATA *ch, char *argument )
{   
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   int race = 0;

   one_argument( argument, arg );

   if ( !HAS_KIT(ch,"ranger") )
   {
       send_to_char("Only rangers have a species enemy.\n\r",ch);
       return;
   }

   if ( ch->species_enemy ) 
   {
       send_to_char("You already selected one!\n\r",ch);
       return;
   }

   if ( ( race = race_lookup( arg ) ) == 0 ||
	!race_table[race].pc_race ||
	race == ch->race )
   {
       send_to_char("That isn't a valid species enemy.\n\r",ch);
       return;
   }

   ch->species_enemy = race_lookup( arg );
   sprintf(buf,"You have selected {C%s{x as your species enemy.",
	   race_table[race].name );
   send_to_char(buf, ch);
   return;
}

 char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
 {
     static char buf[MAX_STRING_LENGTH];
 
     buf[0] = '\0';
 
     if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
     ||  (obj->description == NULL || obj->description[0] == '\0'))
   return buf;

		if(obj->damaged > 0 && obj->carried_by == ch)// Only show with Assess or if you own it
		{
			if(obj->damaged >= 100)
				strcat(buf, "({RBroken{x) ");// Assess not implemented yet (Will show partial damages too)
			else
				strcat(buf, "({DDmgd{x) ");// Dinged up
		}
 
     if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );
     if ( IS_AFFECTED(ch, AFF_DETECT_ALIGN)
    && IS_OBJ_STAT(obj, ITEM_EVIL)   )   
	strcat( buf, "({RRed Aura{x) "  );
     if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)
     &&  IS_OBJ_STAT(obj,ITEM_BLESS))          
	strcat(buf,"({BBlue Aura{x) " );
     if ( obj->item_type == ITEM_WEAPON && IS_SET(obj->value[4], WEAPON_NETHER) )
	 strcat( buf, "({DBlack Aura{x) " );
     if ( IS_AFFECTED(ch, AFF_DETECT_MAGIC)
    && IS_OBJ_STAT(obj, ITEM_MAGIC)  )   strcat( buf, "(Magical) "   );
     if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   
	strcat( buf, "({WGlowing{x) " );
     if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );
 
     if ( fShort )
     {
   if ( obj->short_descr != NULL )
       strcat( buf, obj->short_descr );
     }
     else
     {
   if ( obj->description != NULL)
       strcat( buf, obj->description );
     }
     
     if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM)) {
       char temp_buf[50];
       
       sprintf (temp_buf," [%d]",obj->pIndexData->vnum);
       strcat (buf,temp_buf);
     }    
 
     return buf;
 }
 
 
 
 /*
  * Show a list to a character.
  * Can coalesce duplicated items.
  */
 void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing, bool fExpand )
 {
     char buf[MAX_STRING_LENGTH];
     BUFFER *output;
     char **prgpstrShow;
     int *prgnShow;
     char *pstrShow;
     OBJ_DATA *obj;
     int nShow;
     int iShow;
     int count;
     bool fCombine;
 
     if ( ch->desc == NULL )
   return;
 
     /*
      * Alloc space for output lines.
      */
     output = new_buf();
 
     count = 0;
     for ( obj = list; obj != NULL; obj = obj->next_content )
   count++;
#ifdef OLC_VERSION
     prgpstrShow = alloc_mem( count * sizeof(char *) );
     prgnShow    = alloc_mem( count * sizeof(int)    );
#else
     prgpstrShow = GC_MALLOC( count * sizeof(char *) );
     prgnShow    = GC_MALLOC( count * sizeof(int)    );
#endif
     nShow       = 0;
 
     /*
      * Format the list of objects.
      */
     for ( obj = list; obj != NULL; obj = obj->next_content )
     {
  
   if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )) 
   {
      if (obj->item_type == ITEM_CONTAINER && fExpand )
	 {
	 show_list_to_char(obj->contains, ch, TRUE, TRUE, TRUE);
         }
       pstrShow = format_obj_to_char( obj, ch, fShort );
 
       fCombine = FALSE;
 
       if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE))
       {
     /*
      * Look for duplicates, case sensitive.
      * Matches tend to be near end so run loop backwords.
      */
     for ( iShow = nShow - 1; iShow >= 0; iShow-- )
     {
         if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
         {
       prgnShow[iShow]++;
       fCombine = TRUE;
       break;
         }
     }
       }
 
       /*
        * Couldn't combine, or didn't want to.
        */
       if ( !fCombine )
       {
     prgpstrShow [nShow] = str_dup( pstrShow );
     prgnShow    [nShow] = 1;
     nShow++;
       }
   }
     }
 
     /*
      * Output the formatted list.
      */
     for ( iShow = 0; iShow < nShow; iShow++ )
     {
   if (prgpstrShow[iShow][0] == '\0')
   {
       free_string(prgpstrShow[iShow]);
       continue;
   }
 
   if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE ))
   {
       if ( prgnShow[iShow] != 1 )
       {
     sprintf( buf, "(%2d) ", prgnShow[iShow] );
     add_buf(output,buf);
       }
       else
       {
     add_buf(output,"     ");
       }
   }
   add_buf(output,prgpstrShow[iShow]);
   add_buf(output,"\n\r");
   free_string( prgpstrShow[iShow] );
     }
 
     if ( fShowNothing && nShow == 0 )
     {
   if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE))
       send_to_char( "     ", ch );
   send_to_char( "Nothing.\n\r", ch );
     }
     page_to_char(buf_string(output),ch);
 
     /*
      * Clean up.
      */
     free_buf(output);
     free_mem( prgpstrShow, count * sizeof(char *) );
     free_mem( prgnShow,    count * sizeof(int)    );
 
     return;
 }
 
 
 
 void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
 {
     char buf[MAX_STRING_LENGTH],message[MAX_STRING_LENGTH];
 
     buf[0] = '\0';

     if ( !IS_NPC(victim) && victim->desc == NULL )	strcat( buf, "({YLinkdead{x) ");
     if ( IS_SET(victim->comm,COMM_AFK     )   ) strcat( buf, "[{CAFK{x] "    );
     if ( IS_AFFECTED(victim, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
     if ( victim->invis_level >= LEVEL_HERO    ) strcat( buf, "(Wizi) "       );
     if ( IS_AFFECTED(victim, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
     if ( IS_AFFECTED(victim, AFF_CHARM)       ) strcat( buf, "(Charmed) "    );
     if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) ) 
	strcat( buf, "({MPink Aura{x) " );
     if ( IS_AFFECTED(victim, AFF_FAERIE_FOG) ) strcat( buf, "(Purple Aura) "  );
     if(!IS_SET(victim->mhs,MHS_GLADIATOR) || gladiator_info.exper == FALSE)
     {
     if ( IS_EVIL(victim)
     &&   (IS_AFFECTED(ch, AFF_DETECT_ALIGN) || IS_SET(ch->act, PLR_HOLYLIGHT))     ) 
	strcat( buf, "({RRed Aura{x) " );
     if ( IS_GOOD(victim)
     &&   (IS_AFFECTED(ch, AFF_DETECT_ALIGN) || IS_SET(ch->act, PLR_HOLYLIGHT))     ) 
	strcat( buf, "({YGolden Aura{x) " );
     }// End gladiator hiding
     if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_DWEEB  ) )
             strcat( buf, "({CDWEEB{x) ");

     if (IS_NPC(victim))
     {
     if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
     if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) 
	strcat( buf, "({WWhite Aura{x) " );
     if ( is_affected(victim,skill_lookup("flame shield") ) )
	strcat( buf, "({rFire Shield{x} " );
     if ( is_affected(victim,skill_lookup("frost shield") ) )
	strcat( buf, "({cIce Shield{x} " );
     if ( is_affected(victim,skill_lookup("electric shield") ) )
	strcat( buf, "({yElectric Shield{x} " );
     }
     else
     {
	if(!IS_SET(victim->mhs,MHS_SHAPEMORPHED))
	{
     if ( IS_AFFECTED(victim, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");
     if ( is_affected(victim, skill_lookup("wraithform")) )
	strcat( buf, "({DBlack Aura{x) " );
     if ( IS_AFFECTED(victim, AFF_SANCTUARY)   ) 
	strcat( buf, "({WWhite Aura{x) " );
   if(!IS_SET(victim->mhs,MHS_GLADIATOR) || gladiator_info.exper == FALSE)
   {
     if ( is_affected(victim,skill_lookup("flame shield") ) )
	strcat( buf, "({rFire Shield{x} " );
     if ( is_affected(victim,skill_lookup("frost shield") ) )
	strcat( buf, "({cIce Shield{x} " );
     if ( is_affected(victim,skill_lookup("electric shield") ) )
	strcat( buf, "({yElectric Shield{x} " );
     if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER ) )
             strcat( buf, "({RKILLER{x) "     );
     if ( !IS_NPC(victim) && !IS_SET(victim->act, PLR_KILLER ) 
	 && victim->trumps > 0)
             strcat( buf, "({rTHUG{x) "     );
     if ( !IS_NPC(victim) && IS_SET(victim->wiznet, PLR_RUFFIAN ) 
	&& !IS_SET(victim->act, PLR_KILLER) && victim->trumps == 0 )
             strcat( buf, "({rRUFFIAN{x) "     );
     if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF  ) )
             strcat( buf, "({RTHIEF{x) "      );
   }
       }
     }
             
     if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM)) {
       char temp_buf[50];
         
       if (IS_NPC(victim) && victim->pIndexData) {
         sprintf (temp_buf,"[%d] ",victim->pIndexData->vnum);
         strcat (buf,temp_buf);
       }
     }  
             
     if ( victim->position == victim->start_pos && victim->long_descr[0] != '\0' )
     {
   strcat( buf, victim->long_descr );
   send_to_char( buf, ch );
   return;
     }
 
     if (IS_SET(victim->mhs,MHS_SHAPEMORPHED)
	 || (IS_SET(victim->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE))
     {
	strcat ( buf, victim->long_descr);
     }
     else
     {
        strcat( buf, PERS( victim, ch, FALSE ) );
        if ( !IS_NPC(victim) && !IS_SET(ch->display,DISP_BRIEF_DESCR) 
        &&   victim->position == POS_STANDING && ch->on == NULL )
           strcat( buf, victim->pcdata->title );
     }
 
     switch ( victim->position )
     {
     case POS_DEAD:     strcat( buf, " is DEAD!!" );              break;
     case POS_MORTAL:   strcat( buf, " is mortally wounded." );   break;
     case POS_INCAP:    strcat( buf, " is incapacitated." );      break;
     case POS_STUNNED:  strcat( buf, " is lying here stunned." ); break;
     case POS_SLEEPING: 
   if (victim->on != NULL)
   {
       if (IS_SET(victim->on->value[2],SLEEP_AT))
       {
     sprintf(message," is sleeping at %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else if (IS_SET(victim->on->value[2],SLEEP_ON))
       {
     sprintf(message," is sleeping on %s.",
         victim->on->short_descr); 
     strcat(buf,message);
       }
       else
       {
     sprintf(message, " is sleeping in %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
   }
   else 
       strcat(buf," is sleeping here.");
   break;
     case POS_RESTING:  
   if (victim->on != NULL)
   {
       if (IS_SET(victim->on->value[2],REST_AT))
       {
     sprintf(message," is resting at %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else if (IS_SET(victim->on->value[2],REST_ON))
       {
     sprintf(message," is resting on %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else 
       {
     sprintf(message, " is resting in %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
   }
   else
       strcat( buf, " is resting here." );       
   break;
     case POS_SITTING:  
   if (victim->on != NULL)
   {
       if (IS_SET(victim->on->value[2],SIT_AT))
       {
     sprintf(message," is sitting at %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else if (IS_SET(victim->on->value[2],SIT_ON))
       {
     sprintf(message," is sitting on %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else
       {
     sprintf(message, " is sitting in %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
   }
   else
       strcat(buf, " is sitting here.");
   break;
     case POS_STANDING: 
   if (victim->on != NULL)
   {
       if (IS_SET(victim->on->value[2],STAND_AT))
       {
     sprintf(message," is standing at %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
       else if (IS_SET(victim->on->value[2],STAND_ON))
       {
     sprintf(message," is standing on %s.",
        victim->on->short_descr);
     strcat(buf,message);
       }
       else
       {
     sprintf(message," is standing in %s.",
         victim->on->short_descr);
     strcat(buf,message);
       }
   }
   else
       strcat( buf, " is here." );               
   break;
     case POS_FIGHTING:
   strcat( buf, " is here, fighting " );
   if ( victim->fighting == NULL )
       strcat( buf, "thin air??" );
   else if ( victim->fighting == ch )
       strcat( buf, "YOU!" );
   else if ( victim->in_room == victim->fighting->in_room )
   {
    /*   if (IS_SET(victim->fighting->mhs,MHS_GLADIATOR) 
	   && gladiator_info.blind == TRUE)
          strcat( buf, PERS( victim->fighting->long_descr, ch, FALSE ) );
       else */
     if (IS_SET(victim->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE)
	strcat ( buf, victim->long_descr);
     else       
          strcat( buf, PERS( victim->fighting, ch, FALSE ) ); 
       strcat( buf, "." );
   }
   else
       strcat( buf, "somone who left??" );
   break;
     }
 
     strcat( buf, "\n\r" );
     buf[0] = UPPER(buf[0]);
     send_to_char( buf, ch );
     return;
 }
 
 
 
 void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
 {
     char buf[MAX_STRING_LENGTH];
     OBJ_DATA *obj;
     int iWear;
     int percent;
     bool found;
     AFFECT_DATA *paf, *paf_last = NULL;
 
     if ( can_see( victim, ch, FALSE ) )
     {
   if (ch == victim)
       if(IS_SET(ch->mhs,MHS_SHAPEMORPHED) ||
	  (IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE))
          act( "$l looks at $mself.",ch,NULL,NULL,TO_ROOM,FALSE);
       else
          act( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM,FALSE);
   else
   {
       if(IS_SET(ch->mhs,MHS_SHAPEMORPHED) ||
	  (IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE))
       {
          act( "$l looks at you.", ch, NULL, victim, TO_VICT, FALSE  );
          act( "$l looks at $L.",  ch, NULL, victim, TO_NOTVICT,FALSE );
       }
       else
       {
          act( "$n looks at you.", ch, NULL, victim, TO_VICT, FALSE  );
          act( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT,FALSE );
       }
   }
     }
 
     if (IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind ==TRUE)
	strcpy( buf, ch->long_descr);
     else
     {
	if(!IS_SET(ch->display,DISP_BRIEF_CHAR_DESCR))
	{
        if ( victim->description[0] != '\0' )
           send_to_char( victim->description, ch );
        else
           act( "You see nothing special about $M.", ch, NULL, victim, TO_CHAR, FALSE);
	}
 
        if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM))
	{
           char temp_buf[50];
       
           if (IS_NPC(victim) && victim->pIndexData)
           {
              sprintf (temp_buf,"[%d] ",victim->pIndexData->vnum);
              strcat (buf,temp_buf);
           }
        }
 
        strcpy( buf, PERS(victim, ch, FALSE) );
 
        strcat (buf, " the ");
        if( !IS_NPC(victim) )
        {
           if (IS_SET(victim->act,PLR_WERE))
              strcat (buf,"garou ");
           if (IS_SET (victim->act,PLR_VAMP))
              strcat (buf,"nosferatu ");
           if (IS_SET (victim->act,PLR_MUMMY))
              strcat (buf,"mummified ");
        }

        strcat (buf,race_table[victim->race].name);
     }

     if ( victim->max_hit > 0 )
        percent = ( 100 * victim->hit ) / victim->max_hit;
     else
        percent = -1;

     if (percent >= 100) 
   strcat( buf, " is in excellent condition.\n\r");
     else if (percent >= 90) 
   strcat( buf, " has a few scratches.\n\r");
     else if (percent >= 75) 
   strcat( buf," has some small wounds and bruises.\n\r");
     else if (percent >=  50) 
   strcat( buf, " has quite a few wounds.\n\r");
     else if (percent >= 30)
   strcat( buf, " has some big nasty wounds and scratches.\n\r");
     else if (percent >= 15)
   strcat ( buf, " looks pretty hurt.\n\r");
     else if (percent >= 0 )
   strcat (buf, " is in awful condition.\n\r");
     else
   strcat(buf, " is bleeding to death.\n\r");
 
     buf[0] = UPPER(buf[0]);
     send_to_char( buf, ch );
 
     if(!IS_SET(ch->mhs,MHS_GLADIATOR))
     {
        found = FALSE;
     for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
     {

   if ( ( obj = get_eq_char( victim, iWear ) ) != NULL
   &&   can_see_obj( ch, obj ) )
   {
       if ( !found )
       {
     send_to_char( "\n\r", ch );
     act( "$N is using:", ch, NULL, victim, TO_CHAR, FALSE );
     found = TRUE;
       }
       send_to_char( where_name[iWear], ch );
       send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
       send_to_char( "\n\r", ch );
   }
     }
 
     if ( victim != ch
     &&   !IS_NPC(ch)
     &&   IS_SET(ch->act,PLR_AUTOPEEK)
     &&   number_percent( ) < get_skill(ch,gsn_peek))
     {
   send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
   check_improve(ch,gsn_peek,TRUE,4);
   show_list_to_char( victim->carrying, ch, TRUE, TRUE, FALSE );
     }

     if ( IS_AFFECTED(ch,AFF_DETECT_MAGIC) ||  IS_SET(ch->act, PLR_HOLYLIGHT)) 
     {
  if ( victim->affected != NULL )
  {
     bool vis_dur = FALSE;
     if(victim->level <= ch->level && ch->class == class_lookup("wizard") && number_percent() < get_skill(ch,gsn_spellcraft))
	vis_dur = TRUE;
     send_to_char( "\n\rAffected by the following spells:\n\r", ch );   
     for ( paf = victim->affected; paf != NULL; paf = paf->next )
     {
	if (paf_last != NULL)
	{
           if (paf->type != paf_last->type)
           {
              if(!vis_dur)
		sprintf( buf, "Spell: %-15s\n\r", skill_table[paf->type].name );
	      else
	      {
		if(paf->duration < 0)
		  sprintf(buf, "Spell: %-15s {ypermanently{x\n\r",skill_table[paf->type].name);
		else
		  sprintf(buf, "Spell: %-15s for %d hours\n\r",skill_table[paf->type].name, paf->duration);
	      }
              send_to_char( buf, ch );
	   }
	}
	else
	{
//           sprintf( buf, "Spell: %-15s\n\r", skill_table[paf->type].name );
              if(!vis_dur)
		sprintf( buf, "Spell: %-15s\n\r", skill_table[paf->type].name );
	      else
	      {
		if(paf->duration < 0)
		  sprintf(buf, "Spell: %-15s {ypermanently{x\n\r",skill_table[paf->type].name);
		else
		  sprintf(buf, "Spell: %-15s for %d hours\n\r",skill_table[paf->type].name, paf->duration);
	      }
	   send_to_char( buf, ch );
	}
	paf_last = paf;
     }
  }
           else
           send_to_char("\n\rNot affected by any spells.\n\r",ch);
        }
     }
     return;
 }
 
 
 
 void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
 {
     CHAR_DATA *rch;
 
     for ( rch = list; rch != NULL; rch = rch->next_in_room )
     {
   if ( rch == ch )
       continue;
 
   if ( get_trust(ch) < rch->invis_level)
       continue;

   if ( can_see( ch, rch, FALSE ) )
   {
       show_char_to_char_0( rch, ch );
   }
   else if ( room_is_dark( ch->in_room )
   &&        IS_AFFECTED(rch, AFF_INFRARED ) )
   {
       send_to_char( "You see glowing red eyes watching YOU!\n\r", ch );
   }
     }
 
     return;
 } 
 
 
 
 bool check_blind( CHAR_DATA *ch )
 {
 
     if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
   return TRUE;
 
     if ( IS_AFFECTED(ch, AFF_BLIND) )
     { 
   send_to_char( "You can't see a thing!\n\r", ch ); 
   return FALSE; 
     }
 
     return TRUE;
 }

void do_kit( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char cbuf[MAX_STRING_LENGTH];
   int kit;
   bool fAll = TRUE;

   argument = one_argument( argument, arg1 );
   one_argument( argument, arg2 );

   if ( arg1[0] == '\0' ||
	arg2[0] == '\0' )
   {
       send_to_char(" Syntax:  kit info <kit name>\n\r"
		    "          kit take <kit name>\n\r",ch);
       return;
   }

   if ( !str_cmp( arg1, "take" ) )
   {
	int i;

	if ( ch->level < 10 )
	{
	    send_to_char("You must be level 10 to take a kit.\n\r",ch);
	    return;
	}

	if ( ( kit = kit_lookup(arg2) ) < 0 )
	{
	    send_to_char("Invalid kit.\n\r",ch);
	    return;
	}

	if ( !kit_table[kit].flags )
	{
	    send_to_char("Can you not read?  This Kit isn't ready.\n\r",ch);
	    return;
	}

        if(IS_SET(ch->mhs,MHS_SHAPEMORPHED) ||
           IS_SET(ch->mhs,MHS_SHAPESHIFTED))
        {
	    send_to_char("You can not take a kit while shapeshifted or shapemorphed.\n\r",ch);
	    return;
        }

	/* Are you the right class? */
	if ( !kit_table[kit].class[ch->class] )
	{
	    send_to_char("You are not of the right class.\n\r",ch);
	    return;
	}

	if( !kit_table[kit].race[ch->race] )
	{
	    send_to_char("You are not of the right race.\n\r",ch);
	    return;
	}

	for ( i = 0 ; i < MAX_STATS ; i++ )
	    if ( kit_table[kit].min_stat[i] > 0 &&
		 ch->perm_stat[i] < kit_table[kit].min_stat[i] )
	    {
                if(i == STAT_AGT || i == STAT_END)
                   continue;// Not checking these
		send_to_char("Your attributes are not high enough.\n\r",ch);
		return;
	    }

	    if ( kit == kit_lookup("vampyre hunter") && 
		IS_SET(ch->act,PLR_VAMP)
	       )
            {
                send_to_char("What are you some kind of kin slayer?\n\r",ch);
                return;
            }
            if ( kit == kit_lookup("lycanthrope hunter") &&
                IS_SET(ch->act,PLR_WERE)
               )
            {
                send_to_char("What are you some kind of kin slayer?\n\r",ch);
                return;
            }
            if ( kit == kit_lookup("archeolgist") &&
                IS_SET(ch->act,PLR_MUMMY)
               )
            {
                send_to_char("What are you some kind of kin slayer?\n\r",ch);
                return;
            }

	    /* check buffy and remort status */
	    if ( kit == kit_lookup("buffy") &&
		 ( IS_SET(ch->act, PLR_VAMP) || 
		   IS_SET(ch->act, PLR_WERE) || 
		   IS_SET(ch->act, PLR_MUMMY) )
               )
	    {
		send_to_char("What are you some kind of kin slayer?\n\r",ch);
		return;
	    }

/*
	if ( ch->train < kit_table[kit].cost )
	*/
	if ( ch->practice < (kit_table[kit].cost * 10) )
	{
	    send_to_char("You don't have enough practices.\n\r",ch);
	    return;
	}

	/* WHEW!  All done. */
	/* Remove any existing kit */
	if ( ch->kit )
	{
	   for ( i = 0 ; i < 5 ; i++ )
	      if ( kit_table[ch->kit].skills[i] == NULL )
		  break;
	      else
		{
		  group_remove(ch,kit_table[ch->kit].skills[i]);
		}
		  /* remove any extra special bonuses */
		  if ( ch->kit == kit_lookup("ranger") )
                     ch->species_enemy = 0;
		  if ( ch->kit == kit_lookup("myrmidon") )
		     ch->pcdata->specialize = 0;
	}

	if ( kit == kit_lookup("myrmidon") &&
	     ch->pcdata->specialize != 0 )
ch->practice += (skill_table[ch->pcdata->specialize].rating[ch->class] * 10);
	/*
	ch->train += skill_table[ch->pcdata->specialize].rating[ch->class];
	*/

	ch->kit = kit;
	ch->practice -= (kit_table[kit].cost * 10);
	/*
	ch->train -= kit_table[kit].cost;
	*/
	for ( i = 0 ; i < 5 ; i++ )
	    if ( kit_table[kit].skills[i] == NULL )
		break;
	    else 
		group_add(ch,kit_table[kit].skills[i],FALSE);

	sprintf(buf,"You take the %s kit.\n\r",
		kit_table[kit].name );
	send_to_char(buf,ch);
	if ( kit == kit_lookup("nethermancer") )
	{
	/*
		OBJ_DATA *obj;

		for ( obj = ch->carrying ; obj ; obj = obj->next_content )
		{
		    if ( obj->item_type == ITEM_ARMOR )
			remove_obj( ch, obj, FALSE );
		}
		*/
/* Remove all EQ on the Player to avoid getting around armor restrictions */
		remove_all_objs(ch);

		do_outfit( ch, "nethermancer" );
	}

	return;
   }

   if ( !str_cmp( arg1, "info" ) )
   {
       int i;
       
       if ( ( kit = kit_lookup(arg2) ) < 0 )
       {
	   send_to_char("Invalid kit.  Type 'help kit list' for a list.\n\r",ch);
	   return;
       }
	      
	cbuf[0] = '\0';
       sprintf(buf,"{WKit: {x%s   {WCost: {G%d{x practices\n\r",
		kit_table[kit].name, (kit_table[kit].cost * 10));
		/*
       sprintf(buf,"{WKit: {x%s   {WCost: {G%d{x trains\n\r",
		kit_table[kit].name, kit_table[kit].cost);
		*/
	send_to_char(buf,ch);
	send_to_char("{WClasses{x:  ", ch ); 
	for ( i = 0 ; i < MAX_CLASS ; i++ )
	    if ( kit_table[kit].class[i] )
	    {
		strcat(cbuf, class_table[i].name );
		strcat(cbuf, " " );
 	    }
	    else
	    {
		fAll = FALSE;
	    }
	if ( fAll )
		send_to_char( "all", ch);
	else
		send_to_char( cbuf, ch );

	fAll = TRUE;
	cbuf[0] = '\0';

        send_to_char("\n\r{WRaces{x:  ",ch);
	for ( i = 1 ; i < MAX_PC_RACE ; i++ )
	   if ( kit_table[kit].race[i] )
	   {
	       strcat(cbuf, pc_race_table[i].name );
	       strcat(cbuf, " " );
	   }
	   else
	   {
	       fAll = FALSE;
	   }

	if (  fAll )
		send_to_char( "all", ch );
	else
		send_to_char( cbuf, ch );

	send_to_char("\n\r{WMinimum Attributes:{x  ",ch);
	for ( i = 0 ; i < MAX_STATS ; i++ )
	    if ( kit_table[kit].min_stat[i] > 0 )
	    {
		if(i == STAT_AGT || i == STAT_END)
		   continue;
		char buf2[10]; 

		buf[0] = '\0';
		switch( i )
		{
		case STAT_STR: strcat(buf,"STR "); break; 
		case STAT_DEX: strcat(buf,"DEX "); break;
		case STAT_INT: strcat(buf,"INT "); break;
		case STAT_WIS: strcat(buf,"WIS "); break;
		case STAT_CON: strcat(buf,"CON "); break;
		case STAT_AGT: strcat(buf,"AGT "); break;
 		case STAT_END: strcat(buf,"END "); break;
 		case STAT_SOC: strcat(buf,"CHA "); break;
		default:       strcat(buf,"??? "); break;
		};
		sprintf(buf2,"{G%d{x  ", kit_table[kit].min_stat[i] );
		strcat(buf,buf2);
		send_to_char(buf,ch);
	    }
	send_to_char("\n\r{WSkills/Groups:{x  ",ch);
    	for ( i = 0 ; i < 5 ; i++ )
	    if ( kit_table[kit].skills[i] != NULL )
	    {
		sprintf(buf,"'%s'  ",kit_table[kit].skills[i]);
		send_to_char(buf,ch);
	    }

	if ( !kit_table[kit].flags )
	    send_to_char("\n\r{RNOTICE:{x This kit is not complete.  "
			 "If you take it, you'll probably get screwed.",ch);
	    
	if ( !kit_table[kit].class[ch->class] ||
	     !kit_table[kit].race[ch->race] ||
	     !kit_table[kit].sex[ch->pcdata->true_sex] ) 
	    send_to_char("\n\r{RNOTICE:{x You do not qualify for this kit.",ch);
	else
	for ( i = 0 ; i < MAX_STATS ; i++ )
	{
	    if(i == STAT_AGT || i == STAT_END)
		continue;
	    if ( kit_table[kit].min_stat[i] > 0 &&
	         ch->perm_stat[i] < kit_table[kit].min_stat[i] )
	    {
		send_to_char(
		"\n\r{RNOTICE:{x You do not qualify for this kit.",ch);
		break;
	    }
	}

	    send_to_char("\n\r",ch);
	return;
    }

    send_to_char("Not yet.\n\r",ch);
    return;
}


void do_peek(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *victim;
  char arg[MAX_INPUT_LENGTH];
  char buf[100];

  one_argument(argument,arg);

  if (arg[0] == '\0')
   {
	send_to_char("Peek into who's inventory?\n\r.",ch);
	return;
   }

  if ( ( victim = get_char_room( ch, arg ) ) == NULL )
     {
	send_to_char("You can't seem to find them.\n\r",ch);
	return;
     }

  if( number_percent( ) < get_skill(ch,gsn_peek))
     {
   send_to_char( "\n\rYou peek at their inventory:\n\r", ch );
   check_improve(ch,gsn_peek,TRUE,4);
   show_list_to_char( victim->carrying, ch, TRUE, TRUE, FALSE );

   if ( ( ch->class == class_lookup("thief") || 
          ch->class == class_lookup("rogue") ||
          ch->kit == kit_lookup("fence")) && 
        (is_clan(ch) && number_percent() < 25)
      )
   {
      sprintf(buf,"They appear to have %ld {Ygold{x and %ld {Wsilver{x.\n\r",
              victim->gold,victim->silver);
      send_to_char(buf,ch);
       }



     }
  else
     {
	if(IS_AWAKE(victim) && can_see(victim,ch,FALSE))
 act("$N tried to peek at what you're carrying.", victim,NULL,ch,TO_CHAR,FALSE);
 }

  return;
}

 /* changes your scroll */
 void do_scroll(CHAR_DATA *ch, char *argument)
 {
     char arg[MAX_INPUT_LENGTH];
     char buf[100];
     int lines;
 
     one_argument(argument,arg);
     
     if (arg[0] == '\0')
     {
   if (ch->lines == 0)
       send_to_char("You do not page long messages.\n\r",ch);
   else
   {
       sprintf(buf,"You currently display %d lines per page.\n\r",
         ch->lines + 2);
       send_to_char(buf,ch);
   }
   return;
     }
 
     if (!is_number(arg))
     {
   send_to_char("You must provide a number.\n\r",ch);
   return;
     }
 
     lines = atoi(arg);
 
     if (lines == 0)
     {
   send_to_char("Paging disabled.\n\r",ch);
   ch->lines = 0;
   return;
     }
 
     if (lines < 10 || lines > 100)
     {
   send_to_char("You must provide a reasonable number.\n\r",ch);
   return;
     }
 
     sprintf(buf,"Scroll set to %d lines.\n\r",lines);
     send_to_char(buf,ch);
     ch->lines = lines - 2;
 }
 
 /* RT does socials */
 void do_socials(CHAR_DATA *ch, char *argument)
 {
     char buf[MAX_STRING_LENGTH];
     int iSocial;
     int col;
      
     col = 0;
    
     for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
     {
   sprintf(buf,"%-12s",social_table[iSocial].name);
   send_to_char(buf,ch);
   if (++col % 6 == 0)
       send_to_char("\n\r",ch);
     }
 
     if ( col % 6 != 0)
   send_to_char("\n\r",ch);
     return;
 }
 
 
  
 /* RT Commands to replace news, motd, imotd, etc from ROM */
 
 void do_motd(CHAR_DATA *ch, char *argument)
 {
     do_help(ch,"motd");
 }
 
 void do_imotd(CHAR_DATA *ch, char *argument)
 {  
     do_help(ch,"imotd");
 }
 
 void do_rules(CHAR_DATA *ch, char *argument)
 {
     do_help(ch,"rules");
 }
 
 void do_story(CHAR_DATA *ch, char *argument)
 {
     do_help(ch,"story");
 }
 
 void do_wizlist(CHAR_DATA *ch, char *argument)
 {
     do_help(ch,"wizlist");
 }
 
 void do_roar( CHAR_DATA *ch)
 {
     CHAR_DATA *vch;
     CHAR_DATA *vch_next;
 
     if (!(ch->race == race_lookup("dragon")))
   {
   send_to_char("Grow some scales.\n\r",ch);
   return;
   }
     if (ch->move < 20 || ch->mana <2)
   {
   send_to_char("You just can't build up the energy to roar.\n\r",ch);
   return;
   }
     send_to_char( "You let out an earthshaking ROAR!\n\r", ch );
 act( "$n ROARS and makes your entire body quiver!",ch,NULL,NULL,TO_ROOM,FALSE);
 
     for ( vch = char_list; vch != NULL; vch = vch_next )
     {
   vch_next        = vch->next;
   if ( vch->in_room == NULL )
       continue;
 
   if ( vch->in_room->area == ch->in_room->area )
     send_to_char( "You hear the echoes of a dragon roar in the distance.\n\r"
     "The ground beneath your feet rumbles a bit.\n\r", vch );
     }
     ch->move -= apply_chi(ch,20);
     ch->mana -= 4;
     return;
 }


 /* BS */
 void do_display(CHAR_DATA *ch, char *argument )
 {
    char buf[MAX_STRING_LENGTH];

    send_to_char("-== Display Options ==-\n\r",ch);

    send_to_char("Brief Toggles:\n\r",ch);
    sprintf(buf,"   Descriptions   %s\n\r",
		IS_SET(ch->display,DISP_BRIEF_DESCR) ? "ON" : "OFF" );
    send_to_char(buf,ch);

    sprintf(buf,"   Combat         %s\n\r",
		IS_SET(ch->display,DISP_BRIEF_COMBAT) ? "ON" : "OFF" );
    send_to_char(buf,ch);

    sprintf(buf,"   Who List       %s\n\r",
		IS_SET(ch->display,DISP_BRIEF_WHOLIST) ? "ON" : "OFF" );
    send_to_char(buf,ch);

    sprintf(buf,"   Equipment      %s\n\r",  
		IS_SET(ch->display,DISP_BRIEF_EQLIST) ? "ON" : "OFF" );
    send_to_char(buf,ch);
 
    sprintf(buf,"   Scan           %s\n\r",
		IS_SET(ch->display,DISP_BRIEF_SCAN) ? "ON" : "OFF" );
    send_to_char(buf, ch);

    sprintf(buf,"   Surnames	   %s\n\r",
		IS_SET(ch->display,DISP_SURNAME) ? "ON" : "OFF" );
    send_to_char(buf,ch);

    sprintf(buf,"   Titles	   %s\n\r",
		IS_SET(ch->display,DISP_NOTITLES) ? "OFF" : "ON" );
    send_to_char(buf,ch);

    return;
}

 /* RT this following section holds all the auto commands from ROM, as well as
    replacements for config */
 
 void do_autolist(CHAR_DATA *ch, char *argument)
 {
     /* lists most player flags */
     if (IS_NPC(ch))
       return;
 
     send_to_char("   action     status\n\r",ch);
     send_to_char("---------------------\n\r",ch);
  
     send_to_char("autoassist     ",ch);
     if (IS_SET(ch->act,PLR_AUTOASSIST))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch); 
 
     send_to_char("autoexit       ",ch);
     if (IS_SET(ch->act,PLR_AUTOEXIT))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autogold       ",ch);
     if (IS_SET(ch->act,PLR_AUTOGOLD))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autoloot       ",ch);
     if (IS_SET(ch->act,PLR_AUTOLOOT))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autosac        ",ch);
     if (IS_SET(ch->act,PLR_AUTOSAC))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autosplit      ",ch);
     if (IS_SET(ch->act,PLR_AUTOSPLIT))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autopeek       ",ch);
     if (IS_SET(ch->act,PLR_AUTOPEEK))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("autorecall     ",ch);
     if (IS_SET(ch->act,PLR_NOAUTORECALL))
   send_to_char("OFF\n\r",ch);
     else
   send_to_char("ON\n\r",ch);

     send_to_char("full-sacing    ",ch);
     if (IS_SET(ch->mhs,MHS_FULL_SAC))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("norescue       ",ch);
     if (IS_SET(ch->mhs,MHS_NORESCUE))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);

     send_to_char("nowake         ",ch);
     if (IS_SET(ch->act,PLR_NOWAKE))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("compact mode   ",ch);
     if (IS_SET(ch->display,DISP_COMPACT))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     send_to_char("prompt         ",ch);
     if (IS_SET(ch->display,DISP_PROMPT))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);

     send_to_char("beeptell       ",ch);
     if (IS_SET(ch->comm,COMM_TELL_BEEP))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);    
 
   if (IS_IMMORTAL (ch)) {
     send_to_char("dispvnum       ",ch);
     
     if (IS_SET (ch->display,DISP_DISP_VNUM)) 
       send_to_char("ON\n\r",ch);
     else
       send_to_char("OFF\n\r",ch);
   }  
 
     send_to_char("combine items  ",ch);
     if (IS_SET(ch->display,DISP_COMBINE))
   send_to_char("ON\n\r",ch);
     else
   send_to_char("OFF\n\r",ch);
 
     if (!IS_SET(ch->act,PLR_CANLOOT))
   send_to_char("Your corpse is safe from thieves.\n\r",ch);
     else 
   send_to_char("Your corpse may be looted.\n\r",ch);
 
     if (IS_SET(ch->act,PLR_NOSUMMON))
   send_to_char("You cannot be summoned.\n\r",ch);
     else
   send_to_char("You can be summoned.\n\r",ch);
 
     if (IS_SET(ch->act,PLR_NOCANCEL))
   send_to_char("You cannot be cancelled.\n\r",ch);
     else
   send_to_char("You can be cancelled.\n\r",ch);
   
     if (is_clan(ch) && IS_SET(ch->act,PLR_NOOUTOFRANGE))
	send_to_char("You will not attack out of range.\n\r",ch);
     else
     {
       if(is_clan(ch) && !IS_SET(ch->act,PLR_NOOUTOFRANGE))
	 send_to_char("You may attack out of range.\n\r",ch);
     }
    
     if (IS_SET(ch->act,PLR_NOFOLLOW))
   send_to_char("You do not welcome followers.\n\r",ch);
     else
   send_to_char("You accept followers.\n\r",ch);
 }
 
 void do_autoassist(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
     
     if (IS_SET(ch->act,PLR_AUTOASSIST))
     {
       send_to_char("Autoassist removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOASSIST);
     }
     else
     {
       send_to_char("You will now assist when needed.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOASSIST);
     }
 }
 
 void do_autopeek(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
     
     if (IS_SET(ch->act,PLR_AUTOPEEK))
     {
       send_to_char("Autopeek removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOPEEK);
     }
     else
     {
       send_to_char("You will now peek automatically when looking.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOPEEK);
     }
     return;
 }

 void do_autoexit(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_AUTOEXIT))
     {
       send_to_char("Exits will no longer be displayed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOEXIT);
     }
     else
     {
       send_to_char("Exits will now be displayed.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOEXIT);
     }
 }
 
 void do_autogold(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_AUTOGOLD))
     {
       send_to_char("Autogold removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOGOLD);
     }
     else
     {
       send_to_char("Automatic gold looting set.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOGOLD);
     }
 }
 
 void do_autoloot(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_AUTOLOOT))
     {
       send_to_char("Autolooting removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOLOOT);
     }
     else
     {
       send_to_char("Automatic corpse looting set.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOLOOT);
     }
 }
 
 void do_norescue(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->mhs,MHS_NORESCUE))
     {
       send_to_char("You can no longer be rescued.\n\r",ch);
       REMOVE_BIT(ch->mhs,MHS_NORESCUE);
     }
     else
     {
       send_to_char("You can now be rescued.\n\r",ch);
       SET_BIT(ch->mhs,MHS_NORESCUE);
     }
 }

 void do_full_sac(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->mhs,MHS_FULL_SAC))
     {
       send_to_char("You can no longer sac full containers.\n\r",ch);
       REMOVE_BIT(ch->mhs,MHS_FULL_SAC);
     }
     else
     {
       send_to_char("You can now sac full containers.\n\r",ch);
       SET_BIT(ch->mhs,MHS_FULL_SAC);
     }
 }


 void do_autosac(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_AUTOSAC))
     {
       send_to_char("Autosacrificing removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOSAC);
     }
     else
     {
       send_to_char("Automatic corpse sacrificing set.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOSAC);
     }
 }
 
 void do_autosplit(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_AUTOSPLIT))
     {
       send_to_char("Autosplitting removed.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
     }
     else
     {
       send_to_char("Automatic gold splitting set.\n\r",ch);
       SET_BIT(ch->act,PLR_AUTOSPLIT);
     }
 }
 
 void do_beeptell (CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
 
     if (IS_SET(ch->comm,COMM_TELL_BEEP))     
     {
       send_to_char("Tells will no longer beep.\n\r",ch);
       REMOVE_BIT(ch->comm,COMM_TELL_BEEP); 
     }
     else
     {
       send_to_char("Tells will beep.\n\r",ch);
       SET_BIT(ch->comm,COMM_TELL_BEEP);
     }
 }
 
 void do_brief(CHAR_DATA *ch, char *argument)
 {
     char arg[MAX_INPUT_LENGTH];
     long	flag = DISP_BRIEF_DESCR;

     one_argument( argument, arg );

     if ( !str_prefix(arg,"room") )
        flag = DISP_BRIEF_DESCR;
     else
     if ( !str_prefix(arg,"combat") )
	flag = DISP_BRIEF_COMBAT;
     else
     if ( !str_prefix(arg,"wholist") )
	flag = DISP_BRIEF_WHOLIST;
     else
     if ( !str_prefix(arg,"longeq") )
	flag = DISP_BRIEF_EQLIST;
     else
     if ( !str_prefix(arg, "scan") )
	flag = DISP_BRIEF_SCAN;
     else
     if ( !str_prefix(arg, "chardesc") )
	flag = DISP_BRIEF_CHAR_DESCR;
     else
     if ( !str_prefix(arg, "titles") )
	flag = DISP_NOTITLES;
     else
     if ( !str_prefix(arg, "surname") )
	flag = DISP_SURNAME;
     else
      if( !str_prefix(arg,"olccolor") )
      {
 	flag = MHS_OLC;
 	if( IS_SET(ch->mhs,flag))
 	  REMOVE_BIT(ch->mhs,flag);
 	else
 	  SET_BIT(ch->mhs,flag);
        send_to_char("Ok.\n\r",ch);
	return;
      }
     else
     {
        send_to_char("usage: brief <combat|wholist|longeq|scan|chardesc|titles|surname|olccolor>\n\r",ch);
	return;
     }
     if ( IS_SET(ch->display,flag))
       REMOVE_BIT(ch->display,flag);
     else
       SET_BIT(ch->display,flag);

     send_to_char("Ok.\n\r",ch);

 }
 
 void do_compact(CHAR_DATA *ch, char *argument)
 {
     if (IS_SET(ch->display,DISP_COMPACT))
     {
       send_to_char("Compact mode removed.\n\r",ch);
       REMOVE_BIT(ch->display,DISP_COMPACT);
     }
     else
     {
       send_to_char("Compact mode set.\n\r",ch);
       SET_BIT(ch->display,DISP_COMPACT);
     }
 }
 
 void do_show(CHAR_DATA *ch, char *argument)
 {
     if (IS_SET(ch->display,DISP_SHOW_AFFECTS))
     {
       send_to_char("Affects will no longer be shown in score.\n\r",ch);
       REMOVE_BIT(ch->display,DISP_SHOW_AFFECTS);
     }
     else
     {
       send_to_char("Affects will now be shown in score.\n\r",ch);
       SET_BIT(ch->display,DISP_SHOW_AFFECTS);
     }
 }
 
 void do_prompt(CHAR_DATA *ch, char *argument)
 {
    char buf[MAX_STRING_LENGTH];
  
    if ( argument[0] == '\0' )
    {
   if (IS_SET(ch->display,DISP_PROMPT))
   {
       send_to_char("You will no longer see prompts.\n\r",ch);
       REMOVE_BIT(ch->display,DISP_PROMPT);
   }
   else
   {
       send_to_char("You will now see prompts.\n\r",ch);
       SET_BIT(ch->display,DISP_PROMPT);
   }
        return;
    }
  
    if( !strcmp( argument, "all" ) )
       strcpy( buf, "%h %m %v%c> ");
    else
    {
       if ( strlen(argument) > 50 )
    argument[50] = '\0';
       strcpy( buf, argument );
       smash_tilde( buf );
       if (str_suffix("%c",buf))
   strcat(buf," ");
   
    }
  
    free_string( ch->prompt );
    ch->prompt = str_dup( buf );
    sprintf(buf,"Prompt set to %s\n\r",ch->prompt );
    send_to_char(buf,ch);
    return;
 }
 
 void do_combine(CHAR_DATA *ch, char *argument)
 {
     if (IS_SET(ch->display,DISP_COMBINE))
     {
       send_to_char("Long inventory selected.\n\r",ch);
       REMOVE_BIT(ch->display,DISP_COMBINE);
     }
     else
     {
       send_to_char("Combined inventory selected.\n\r",ch);
       SET_BIT(ch->display,DISP_COMBINE);
     }
 }
 
 void do_noloot(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_CANLOOT))
     {
       send_to_char("Your corpse is now safe from thieves.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_CANLOOT);
     }
     else
     {
       send_to_char("Your corpse may now be looted.\n\r",ch);
       SET_BIT(ch->act,PLR_CANLOOT);
     }
 }
 
 void do_nofollow(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
       return;
  
     if (IS_SET(ch->act,PLR_NOFOLLOW))
     {
       send_to_char("You now accept followers.\n\r",ch);
       REMOVE_BIT(ch->act,PLR_NOFOLLOW);
     }
     else
     {
       send_to_char("You no longer accept followers.\n\r",ch);
       SET_BIT(ch->act,PLR_NOFOLLOW);
       //putting in dismounting
       if( is_mounted(ch) )
       {
         do_dismount(ch," ");
       }
       if(!IS_AFFECTED(ch,AFF_CHARM))
       die_follower( ch );
     }
 }
 
 void do_nosummon(CHAR_DATA *ch, char *argument)
 {
     if (IS_NPC(ch))
     {
       if (IS_SET(ch->imm_flags,IMM_SUMMON))
       {
   send_to_char("You are no longer immune to summon.\n\r",ch);
   REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
       }
       else
       {
   send_to_char("You are now immune to summoning.\n\r",ch);
   SET_BIT(ch->imm_flags,IMM_SUMMON);
       }
     }
     else
     {
       if (IS_SET(ch->act,PLR_NOSUMMON))
       {
   send_to_char("You are no longer immune to summon.\n\r",ch);
   REMOVE_BIT(ch->act,PLR_NOSUMMON);
       }
       else
       {
   send_to_char("You are now immune to summoning.\n\r",ch);
   SET_BIT(ch->act,PLR_NOSUMMON);
       }
     }
 }
 
 void do_nocancel (CHAR_DATA *ch, char *argument)
 {
       if (IS_SET(ch->act,PLR_NOCANCEL)) {
   send_to_char("You are no longer immune to canceling.\n\r",ch);
   REMOVE_BIT(ch->act,PLR_NOCANCEL);
       } else  {
   send_to_char("You are now immune to canceling.\n\r",ch);
   SET_BIT(ch->act,PLR_NOCANCEL);
       }
 }

 void do_nooutofrange (CHAR_DATA *ch, char *argument)
 {
       if (IS_SET(ch->act,PLR_NOOUTOFRANGE)) {
   send_to_char("You may now attack more than 8 levels above you.\n\r",ch);
   REMOVE_BIT(ch->act,PLR_NOOUTOFRANGE);
       } else  {
   send_to_char("You will no longer attack more than 8 levels above you.\n\r",ch);
   SET_BIT(ch->act,PLR_NOOUTOFRANGE);
       }
 }

void do_longeq (CHAR_DATA *ch, char *argument)
{
        if (!IS_SET(ch->display,DISP_BRIEF_EQLIST))
        {
         send_to_char("Short equipment info activated.\n\r",ch);
         SET_BIT(ch->display,DISP_BRIEF_EQLIST);
        }
        else
        {
         send_to_char("Long equipment info activated.\n\r",ch);
         REMOVE_BIT(ch->display, DISP_BRIEF_EQLIST);
	}
}

void do_norecall (CHAR_DATA *ch, char *argument)
{
        if (IS_SET(ch->act,PLR_NOAUTORECALL))
        {
         send_to_char("You will automatically recall if attacked "
                          "when link-dead.\n\r",ch);
         REMOVE_BIT(ch->act,PLR_NOAUTORECALL);
        }
        else
        {
         send_to_char("You will NOT recall if attacked when "
                        "link-dead.\n\r",ch);
         SET_BIT(ch->act,PLR_NOAUTORECALL);
	}
}

void do_nowake (CHAR_DATA *ch, char *argument)
{
        if (IS_SET(ch->act,PLR_NOWAKE))
        {
         send_to_char("You can now be awakened.\n\r",ch);
         REMOVE_BIT(ch->act,PLR_NOWAKE);
        }
        else
        {
         send_to_char("You can NOT be awakened.\n\r",ch);
         SET_BIT(ch->act,PLR_NOWAKE);
	}
}

void do_glance ( CHAR_DATA *ch, char *argument )
 {
     CHAR_DATA *victim;
     char buf  [MAX_STRING_LENGTH];
     char arg [MAX_INPUT_LENGTH];
     int percent;

     if ( ch->desc == NULL )
   return;

     if ( ch->position < POS_SLEEPING )
     {
   send_to_char( "You can't see anything but stars!\n\r", ch );
   return;
     }

     if ( ch->position == POS_SLEEPING )
     {
   send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
   return;
     }

     if ( !check_blind( ch ) )
   return;

     one_argument( argument, arg );

     if ( arg[0] == '\0' )
     {
   send_to_char( "Glance at whom?\n\r", ch);
   return;
     }

     if ( ( victim = get_char_room( ch, arg ) ) != NULL )
   {
     if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE)
	strcpy (buf, victim->long_descr);
     else
     {
        strcpy( buf, PERS(victim, ch, FALSE) );


     strcat (buf, " the ");
     if( !IS_NPC(victim) )
     {
       if (IS_SET(victim->act,PLR_WERE))
  	strcat (buf,"garou ");
       if (IS_SET (victim->act,PLR_VAMP))
  	strcat (buf,"nosferatu ");
       if (IS_SET (victim->act,PLR_MUMMY))
  	strcat (buf,"mummified ");
     }
        strcat (buf,race_table[victim->race].name);
     }

     if ( victim->max_hit > 0 )
   percent = ( 100 * victim->hit ) / UMAX(1,victim->max_hit);
     else
   percent = -1;

     if (percent >= 100)
   strcat( buf, " is in excellent condition.\n\r");
     else if (percent >= 90)
   strcat( buf, " has a few scratches.\n\r");
     else if (percent >= 75)
   strcat( buf," has some small wounds and bruises.\n\r");
     else if (percent >=  50)
   strcat( buf, " has quite a few wounds.\n\r");
     else if (percent >= 30)
   strcat( buf, " has some big nasty wounds and scratches.\n\r");
     else if (percent >= 15)
   strcat ( buf, " looks pretty hurt.\n\r");
     else if (percent >= 0 )
   strcat (buf, " is in awful condition.\n\r");
     else
   strcat(buf, " is bleeding to death.\n\r");

     buf[0] = UPPER(buf[0]);
     send_to_char( buf, ch );
   }
     else
   {
   send_to_char( "You can't seem to find them.\n\r", ch);
   }

   return;
}

 
 void do_look( CHAR_DATA *ch, char *argument )
 {
     char buf  [MAX_STRING_LENGTH];
     char arg1 [MAX_INPUT_LENGTH];
     char arg2 [MAX_INPUT_LENGTH];
     char arg3 [MAX_INPUT_LENGTH];
     EXIT_DATA *pexit;
     CHAR_DATA *victim;
     OBJ_DATA *obj;
     char *pdesc;
     int door;
     int number,count;
 
     if ( ch->desc == NULL )
   return;
 
     if ( ch->position < POS_SLEEPING )
     {
   send_to_char( "You can't see anything but stars!\n\r", ch );
   return;
     }
 
     if ( ch->position == POS_SLEEPING )
     {
   send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
   return;
     }
 
     if ( !check_blind( ch ) )
   return;
 
     if ( !IS_NPC(ch)
     &&   !IS_SET(ch->act, PLR_HOLYLIGHT)
     &&   room_is_dark( ch->in_room ) )
     {
   send_to_char( "It is pitch black ... \n\r", ch );
   show_char_to_char( ch->in_room->people, ch );
   return;
     }
 
     argument = one_argument( argument, arg1 );
     argument = one_argument( argument, arg2 );
     number = number_argument(arg1,arg3);
     count = 0;
 
     if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
     {
   /* 'look' or 'look auto' */
   sprintf(buf,"{c%s{x", ch->in_room->name );
   send_to_char( buf, ch );
   
   if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM)) {
     char temp_buf[50];      
     
     sprintf (temp_buf," [%d] ",ch->in_room->vnum);
     send_to_char (temp_buf,ch);
   }
   
   send_to_char( "\n\r", ch );
 
   if ( arg1[0] == '\0'
   || ( !IS_NPC(ch) && !IS_SET(ch->display, DISP_BRIEF_DESCR)))
   {
       send_to_char( "  ",ch);
       send_to_char( ch->in_room->description, ch );
   //}

     if ( IS_OUTSIDE(ch) && (number_range(0,100) < 10 ) )
     {
       switch(weather_info.sky)
	{
	case SKY_CLOUDLESS:
          if (weather_info.change >= 0)
	  {
          send_to_char("{bA warm southerly breeze warms your chilled bones.{x\r\n",ch);
	  } 
	  else
	  {
          send_to_char("{bA cold northerly wind howls around you.\r\n{x",ch);
	  }
          break;
	case SKY_CLOUDY:
	  if (weather_info.change >= 0)
	  {
	  send_to_char("{bThe clouds seem to be breaking up{x\r\n",ch);
          }
	  else
	  {
	  send_to_char("{bThe clouds seem to be thickening, looks like rains coming.{x\r\n",ch);
	  }
	  break;
	case SKY_RAINING:
          if( weather_info.change >= 0 )
	  {
          send_to_char("{bA warm tropical rain falls from the heavens.{x\r\n",ch);
	  }
	  else
	  {
          send_to_char("{bA cold northerly wind blows the rain into your face.{x\r\n",ch);
	  }
       	  break;
	case SKY_LIGHTNING:
          if (weather_info.change >= 0 )
	  {
          send_to_char("{YWHOA!{x{b That lightning looks dangerous.  Thankfully the worse has passed.{x\r\n",ch);
	  }
	  else
	  {
          send_to_char("{YWHOA!{x{b That lightning looks very dangerous.{x\r\n",ch);
	  }
          break;
	default:
	  break;
	}
     } //end of weather messages
   } //this was added when the room desc was added

         if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT) )
   {
       send_to_char("\n\r",ch);
             do_exits( ch, "auto" );
   }
 
   show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE, FALSE );
   show_char_to_char( ch->in_room->people,   ch );
   return;
     }
 
     if ( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
     {
   /* 'look in' */
   if ( arg2[0] == '\0' )
   {
       send_to_char( "Look in what?\n\r", ch );
       return;
   }
 
   if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
   {
       send_to_char( "You do not see that here.\n\r", ch );
       return;
   }
 
   switch ( obj->item_type )
   {
   default:
       send_to_char( "That is not a container.\n\r", ch );
       break;
 
   case ITEM_DRINK_CON:
       if ( obj->value[1] <= 0 )
       {
     send_to_char( "It is empty.\n\r", ch );
     break;
       }
 
       sprintf( buf, "It's %s full of a %s liquid.\n\r",
     obj->value[1] <     obj->value[0] / 4
         ? "less than" :
     obj->value[1] < 3 * obj->value[0] / 4
         ? "about"     : "more than",
     liq_table[obj->value[2]].liq_color
     );
 
       send_to_char( buf, ch );
       break;
 
   case ITEM_CONTAINER:
   case ITEM_CORPSE_NPC:
   case ITEM_CORPSE_PC:
   case ITEM_FORGE:
       if ( IS_SET(obj->value[1], CONT_CLOSED) && obj->item_type != ITEM_FORGE )
       {
     send_to_char( "It is closed.\n\r", ch );
     break;
       }
 
       act( "$p contains:", ch, obj, NULL, TO_CHAR, FALSE );
       show_list_to_char( obj->contains, ch, TRUE, TRUE, FALSE );
       break;
   }
   return;
     }
 
     if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
     {
        show_char_to_char_1( victim, ch );
        return;
     }
 
     for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
     {

   if ( can_see_obj( ch, obj ) )
   {
       pdesc = get_extra_descr( arg3, obj->extra_descr );
       if ( pdesc != NULL )
         if (++count == number)
         {
         send_to_char( pdesc, ch );
         /* LORE */
         if (!IS_NPC(ch) &&
       number_percent( ) < get_skill(ch,gsn_lore))
           {
       if ( ( number_percent( ) * number_percent( ) ) < 40 
	    && !IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
         {
	   if(obj->pIndexData->cost == obj->cost)
	   {
            obj->cost += ( get_skill(ch,gsn_lore) * 
             0.001 * obj->cost);
           send_to_char( "Your understanding of the lore behind it increases its worth!\n\r",  ch );
	   }
         }
       if (ch->mana >= 20){
       spell_identify(gsn_lore,
           (4* obj->level)/3,ch,obj,TARGET_OBJ);
       ch->mana -= 20;
       check_improve(ch,gsn_lore,TRUE,4);}
           } 
         return;
         }
       else continue;
       pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
       if ( pdesc != NULL )
         if (++count == number)
         { 
         send_to_char( pdesc, ch );
         /* LORE */
         if (!IS_NPC(ch) &&
       number_percent( ) <= get_skill(ch,gsn_lore))
           {
       if ( ( number_percent( ) * number_percent( ) ) < 40 
	    && !IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
         {
           if(obj->pIndexData->cost == obj->cost)
           {
           obj->cost += ( get_skill(ch,gsn_lore) *
             0.001 * obj->cost);
           send_to_char( "Your understanding of the lore behind it increases its worth!\n\r",  ch );
	   }
         }
       if(ch->mana >= 20){
       spell_identify(gsn_lore,
           (4* obj->level)/3,ch,obj,TARGET_OBJ);
       ch->mana -= 20;
       check_improve(ch,gsn_lore,TRUE,4);}
           }         
         return;
         }
       else continue;
 
       if ( is_name( arg3, obj->name ) )
         if (++count == number)
         {
         
           if (IS_IMMORTAL (ch) && IS_SET(ch->display,DISP_DISP_VNUM)) {
             sprintf (buf,"%s [%d]\n\r",obj->description,obj->pIndexData->vnum);
           } else {
             sprintf (buf,"%s\n\r",obj->description);
           }        
           send_to_char( obj->description, ch );          
           send_to_char( "\n\r",ch);
         /* LORE */
         if (!IS_NPC(ch) &&
       number_percent( ) <= get_skill(ch,gsn_lore))
           {
       if ( ( number_percent( ) * number_percent( ) ) < 40 
	    && !IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
         {
           if(obj->pIndexData->cost == obj->cost)
           {

           obj->cost += ( get_skill(ch,gsn_lore) *
             0.001 * obj->cost);
           send_to_char( "Your understanding of the lore behind it increases its worth!\n\r",  ch );
	   }
         }
                         if(ch->mana >= 20){
       spell_identify(gsn_lore,
           (4* obj->level)/3,ch,obj,TARGET_OBJ);
       ch->mana -= 20;
       check_improve(ch,gsn_lore,TRUE,4);}
           }           
           return;
         }
   }
     }
 
     for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
     {
   if ( can_see_obj( ch, obj ) )
   {
       pdesc = get_extra_descr( arg3, obj->extra_descr );
       if ( pdesc != NULL )
         if (++count == number)
         {
         send_to_char( pdesc, ch );
         return;
         }
 
       pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
       if ( pdesc != NULL )
         if (++count == number)
         {
         send_to_char( pdesc, ch );
         return;
         }
   }
 
   if ( is_name( arg3, obj->name ) )
       if (++count == number)
       {
         send_to_char( obj->description, ch );
         send_to_char("\n\r",ch);
         return;
       }
     }
     
     if (count > 0 && count != number)
     {
       if (count == 1)
           sprintf(buf,"You only see one %s here.\n\r",arg3);
       else
           sprintf(buf,"You only see %d %s's here.\n\r",count,arg3);
       
       send_to_char(buf,ch);
       return;
     }
 
     pdesc = get_extra_descr( arg1, ch->in_room->extra_descr );
     if ( pdesc != NULL )
     {
   send_to_char( pdesc, ch );
   return;
     }
 
          if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
     else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
     else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
     else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
     else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
     else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
     else
     {
   send_to_char( "You do not see that here.\n\r", ch );
   return;
     }
 
     /* 'look direction' */
     if ( ( pexit = ch->in_room->exit[door] ) == NULL )
     {
   send_to_char( "Nothing special there.\n\r", ch );
   return;
     }
  
     if ( IS_SET(pexit->exit_info,EX_SECRET)  )
     {
    send_to_char("Nothing special there.\n\r", ch );
	return;
     }

    /* send_to_char( "Nothing special there.\n\r", ch ); */
 
   if ( IS_SET(pexit->exit_info, EX_CLOSED) )
   {
     if ( pexit->keyword    != NULL
       &&   pexit->keyword[0] != '\0'
       &&   pexit->keyword[0] != ' ' )
       {
         act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR, FALSE );
       } else {
         act( "The exit is closed.", ch, NULL, pexit->keyword, TO_CHAR, FALSE );
       }
   } else {
      ROOM_INDEX_DATA *temp_room;
      int temp_brief;
      
      temp_brief = IS_SET(ch->display, DISP_BRIEF_DESCR);
      SET_BIT(ch->display, DISP_BRIEF_DESCR);
      temp_room = ch->in_room;
      ch->in_room = ch->in_room->exit[door]->u1.to_room;
      if (ch->in_room != NULL) {
        do_look (ch,"auto");
      } else {
        bug ("NULL in do_look.",0);
      }
      ch->in_room = temp_room;
      if (!temp_brief) REMOVE_BIT (ch->display, DISP_BRIEF_DESCR);
    } 
 
     return;
 }
 
 /* RT added back for the hell of it */
 void do_read (CHAR_DATA *ch, char *argument )
 {
     do_look(ch,argument);
 }
 
 void do_examine( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
     char arg[MAX_INPUT_LENGTH];
     OBJ_DATA *obj;
 
     one_argument( argument, arg );
 
     if ( arg[0] == '\0' )
     {
   send_to_char( "Examine what?\n\r", ch );
   return;
     }
 
     do_look( ch, arg );
 
     if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
     {
   switch ( obj->item_type )
   {
   default:
       break;
   
/*   case ITEM_JUKEBOX:
       do_play(ch,"list");
       break;
 */ 
   case ITEM_MONEY:
       if (obj->value[0] == 0)
       {
     if (obj->value[1] == 0)
         sprintf(buf,"Odd...there's no coins in the pile.\n\r");
     else if (obj->value[1] == 1)
         sprintf(buf,"Wow. One gold coin.\n\r");
     else
         sprintf(buf,"There are %d gold coins in the pile.\n\r",
       obj->value[1]);
       }
       else if (obj->value[1] == 0)
       {
     if (obj->value[0] == 1)
         sprintf(buf,"Wow. One silver coin.\n\r");
     else
         sprintf(buf,"There are %d silver coins in the pile.\n\r",
       obj->value[0]);
       }
       else
     sprintf(buf,
         "There are %d gold and %d silver coins in the pile.\n\r",
         obj->value[1],obj->value[0]);
       send_to_char(buf,ch);
       break;
 
   case ITEM_DRINK_CON:
   case ITEM_CONTAINER:
   case ITEM_CORPSE_NPC:
   case ITEM_CORPSE_PC:
   case ITEM_FORGE:
       sprintf(buf,"in %s",argument);
       do_look( ch, buf );
   }
     }
 
     return;
 }
 
 
 
 /*
  * Thanks to Zrin for auto-exit part.
  */
 void do_exits( CHAR_DATA *ch, char *argument )
 {
     extern char * const dir_name[];
     char buf[MAX_STRING_LENGTH];
     EXIT_DATA *pexit;
     bool found;
     bool fAuto;
     int door;
 
     fAuto  = !str_cmp( argument, "auto" );
 
     if ( !check_blind( ch ) )
   return;
 
     if (fAuto)
   sprintf(buf,"[{WExits{x: {g");
     else if (IS_IMMORTAL(ch))
   sprintf(buf,"Obvious exits from room %d: {g\n\r",ch->in_room->vnum);
     else
   sprintf(buf,"Obvious exits: {g\n\r");
     found = FALSE;
     for ( door = 0; door <= 5; door++ )
     {
   if ( ( pexit = ch->in_room->exit[door] ) != NULL
   &&   pexit->u1.to_room != NULL
   &&   can_see_room(ch,pexit->u1.to_room) 
   &&   is_room_clan(ch,pexit->u1.to_room)
   &&   !IS_SET(pexit->exit_info, EX_CLOSED)
   &&   !IS_SET(pexit->exit_info, EX_CONCEALED)
   &&	!IS_SET(pexit->exit_info, EX_SECRET) )
   {
       found = TRUE;
       if ( fAuto )
       {
     strcat( buf, " " );
     strcat( buf, dir_name[door] );
       }
       else
       {
     sprintf( buf + strlen(buf), "%-5s - %s",
         capitalize( dir_name[door] ),
         (room_is_dark( pexit->u1.to_room )  && !IS_SET(ch->act, PLR_HOLYLIGHT))
       ?  "Too dark to tell"
       : pexit->u1.to_room->name
         );
 
     if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM)) {
       char temp_buf[50];
     
       sprintf (temp_buf," [%d] ",pexit->u1.to_room->vnum);
       strcat (buf,temp_buf);
     }                
     strcat (buf,"\n\r");
           
       }
   }
     }
 
     if ( !found )
   strcat( buf, fAuto ? " none" : "None.\n\r" );
 
     if ( fAuto )
   strcat( buf, "{x]\n\r" );
 
     send_to_char( buf, ch );
     send_to_char( "{x", ch );
     return;
 }
 
 void do_worth( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
 
     if (IS_NPC(ch))
     {
   sprintf(buf,"You have %ld {Ygold{x and %ld {Wsilver{x.\n\r",
       ch->gold,ch->silver);
   send_to_char(buf,ch);
   return;
     }
 
    sprintf(buf, 
    "You have %ld gold, %ld silver, and %d experience (%d exp to level).\n\r",
   ch->gold, ch->silver,ch->exp,
   (ch->level + 1) * exp_per_level(ch,ch->pcdata->points) - ch->exp);
 
     send_to_char(buf,ch);
 
     return;
 }


void do_attributes( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];

     sprintf( buf, "Str: %d/%d Int: %d/%d Wis: %d/%d "
	"Dex: %d/%d Con: %d/%d Cha: %d/%d",
//     sprintf( buf, "Str: %d/%d Int: %d/%d Wis: %d/%d Soc: %d/%d\n\r"
//	"Dex: %d/%d Agt: %d/%d End: %d/%d Con: %d/%d\n\r",
   ch->perm_stat[STAT_STR],
   get_curr_stat(ch,STAT_STR),
   ch->perm_stat[STAT_INT],
   get_curr_stat(ch,STAT_INT),
   ch->perm_stat[STAT_WIS],
   get_curr_stat(ch,STAT_WIS),
   ch->perm_stat[STAT_DEX],
   get_curr_stat(ch,STAT_DEX),
//   ch->perm_stat[STAT_AGT],
//   get_curr_stat(ch,STAT_AGT),
//   ch->perm_stat[STAT_END],    
//   get_curr_stat(ch,STAT_END),
   ch->perm_stat[STAT_CON],
   get_curr_stat(ch,STAT_CON),
   ch->perm_stat[STAT_SOC],
   get_curr_stat(ch,STAT_SOC) );

   if ( argument[0] == '\0' )
   {
      strcat(buf, "\n\r");
      send_to_char("Your attributes are:\n\r",ch);
      send_to_char(buf,ch);
      return;
   }
   else
   if ( !str_cmp(argument,"say") )
   {
      char buf2[MAX_STRING_LENGTH];

      sprintf(buf2,"$n says 'My stats:\n\r%s'",buf);
      act(buf2,ch,NULL,NULL,TO_ROOM,TRUE);

      sprintf(buf2,"You say 'My stats:\n\r%s'",buf);
      act(buf2,ch,NULL,NULL,TO_CHAR,TRUE);
      return;
   }
  return;
}

void do_cscore( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];

     if(!IS_NPC(ch) && is_clan(ch))
	{
	sprintf( buf, "Kills: Lower: %d Equal: %d Greater: %d\n\r", 
		ch->pcdata->killer_data[PC_LOWER_KILLS],
		ch->pcdata->killer_data[PC_EQUAL_KILLS],
                ch->pcdata->killer_data[PC_GREATER_KILLS]);
	send_to_char(buf, ch);
	if (ch->pcdata->steal_data[PC_STOLEN_ITEMS] > 0
	    || ch->pcdata->steal_data[PC_STOLEN_GOLD] > 0) 
	{
	sprintf( buf, "Stolen: %ld items %ld gold",  
		ch->pcdata->steal_data[PC_STOLEN_ITEMS],
		ch->pcdata->steal_data[PC_STOLEN_GOLD]); 
	send_to_char(buf, ch);
	}
	if (ch->pcdata->steal_data[PC_SLICES] > 0)
	{
	sprintf( buf, "Slices: %ld  ", 
		ch->pcdata->steal_data[PC_SLICES]);
	send_to_char(buf, ch);
	}
	if( ch->pcdata->outcT > 0 || ch->pcdata->ruffT )
	{
	  sprintf( buf, "Outcast ticks: %d  Ruffian ticks: %d\n\r",
	  ch->pcdata->outcT,ch->pcdata->ruffT);
	  send_to_char(buf,ch);
	}
        else
        { send_to_char("\n\r",ch); }
      }
      else
       { send_to_char("You're not clanned.\n\r", ch); }
     return;
}

void do_score( CHAR_DATA *ch, char *argument )
{
     char buf[MAX_STRING_LENGTH];
     ROOM_INDEX_DATA *location; 
     int i,percent;
     char wound[100];
     char mental[100];
     char moves[100];

      if (ch->max_hit > 0)
    percent = ch->hit * 100 / ch->max_hit;
      else
    percent = -1;

      if(is_affected(ch, gsn_rage))
    sprintf(wound, "You are in battle-rage.\n\r");
      else if (percent >= 100)
    sprintf(wound,"You are in excellent condition.\n\r");
      else if (percent >= 90)
    sprintf(wound,"You have a few scratches.\n\r");
      else if (percent >= 75)
    sprintf(wound,"You have some small wounds and bruises.\n\r");
      else if (percent >= 50)
    sprintf(wound,"You have quite a few wounds.\n\r");
      else if (percent >= 30)
    sprintf(wound,"You have some big nasty wounds and scratches.\n\r");
      else if (percent >= 15)
    sprintf(wound,"You are pretty hurt.\n\r");
      else if (percent >= 0)
    sprintf(wound,"You are in awful condition.\n\r");
      else
    sprintf(wound,"You are bleeding to death.\n\r");

      if (ch->max_mana > 0)
    percent = ch->mana * 100 / ch->max_mana;
      else
    percent = -1;

      if (percent >= 100)
    sprintf(mental,"You are mentally fit.\n\r");
      else if (percent >= 90)
    sprintf(mental,"You are a bit slow.\n\r");
      else if (percent >= 75)
    sprintf(mental,"You have some mental lapses.\n\r");
      else if (percent >= 50)
    sprintf(mental,"You are quite drained.\n\r");
      else if (percent >= 30)
    sprintf(mental,"You feel dazed.\n\r");
      else if (percent >= 15)
    sprintf(mental,"You are nearly spent.\n\r");
      else if (percent >= 0)
    sprintf(mental,"You are brain dead.\n\r");
      else
    sprintf(mental,"You are helpless.\n\r");

      if (ch->max_move > 0)
    percent = ch->move * 100 / ch->max_move;
      else
    percent = -1;

      if (percent >= 100)
    sprintf(moves,"You are full of energy.\n\r");
      else if (percent >= 90)
    sprintf(moves,"You are energetic.\n\r");
      else if (percent >= 75)
    sprintf(moves,"You are breathing hard.\n\r");
      else if (percent >= 50)
    sprintf(moves,"Your heart is pounding.\n\r");
      else if (percent >= 30)
    sprintf(moves,"You feel winded.\n\r");
      else if (percent >= 15)
    sprintf(moves,"You are cramping up.\n\r");
      else if (percent >= 0)
    sprintf(moves,"You are exhausted.\n\r");
      else
    sprintf(moves,"You are motionless.\n\r");



 /*
     sprintf( buf,
   "You are %s%s%s, level %d(%d), %d years old (%d hours).\n\r",
   IS_SET(ch->mhs,MHS_SAVANT) ? "a Savant" : ch->name,
   ch->level, IS_NPC(ch) ? 0 : ch->pcdata->debit_level, get_age(ch),
   ( ch->played + (int) (current_time - ch->logon) ) / 3600 );
 */

     sprintf( buf,
   "You are %s %s, %d years old (%d hours).\n\r%s%s%s",
   ch->name, IS_NPC(ch) ? "" : ch->pcdata->surname,
   get_age(ch), ( ch->played + (int) (current_time - ch->logon) ) / 3600,
   wound, mental, moves );
     send_to_char( buf, ch );

/*   if ( ch->position == POS_FIGHTING )
   {
     send_to_char("{RStats not available during combat.{x\r\n",ch);
   }
   else
*/   {
  if (is_affected(ch,gsn_rage))
     sprintf(buf, "You have ??? {Yhit points{x, %d of %d {Gmana{x, %d of %d {Bmoves{x.\n\r",
    	ch->mana,ch->max_mana,ch->move,ch->max_move);
  else
     sprintf(buf, "You have %d of %d {Yhit points{x, %d of %d {Gmana{x, %d of %d {Bmoves{x.\n\r",
      ch->hit,ch->max_hit,ch->mana,ch->max_mana,ch->move,ch->max_move);
     send_to_char(buf, ch);
   }

     if ( get_trust( ch ) != ch->level )
     {
   sprintf( buf, "You are trusted at level %d.\n\r",
       get_trust( ch ) );
   send_to_char( buf, ch );
     }
 
     sprintf(buf, "Race: %s  Sex: %s  Class: %s %s  Kit: %s\n\r",
   race_table[ch->race].name,
   ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
   IS_NPC(ch) ? "" : 
      ( class_table[ch->class].reclass ?
       class_table[ch->pcdata->old_class].name : "" ),
   IS_NPC(ch) ? "mobile" : class_table[ch->class].name,
   !ch->kit ? "none" : kit_table[ch->kit].name );
     send_to_char(buf,ch);
  
   if ( !IS_NPC(ch) )
   {
      if(ch->pcdata->retrain)
        sprintf( buf,
   "Trains: %d  Retrains: %d  Practices: %d  Skill Points: %d  Bounty: %ld ",
   ch->train, ch->pcdata->retrain, ch->practice, ch->skill_points, ch->pcdata->bounty );
      else
     sprintf( buf,
   "Trains: %d  Practices: %d  Skill Points: %d  Bounty: %ld ",
   ch->train, ch->practice, ch->skill_points, ch->pcdata->bounty );
     send_to_char( buf, ch );
     send_to_char("\n\r",ch);
    if(ch->pcdata->half_retrain != 0 || ch->pcdata->half_train != 0)
    {
     sprintf(buf, "Half Trains: %d  Half Retrains: %d\n\r", ch->pcdata->half_train, ch->pcdata->half_retrain);
     send_to_char( buf, ch );
    }
   }

     if(!IS_NPC(ch) && IS_SET(ch->mhs,MHS_HIGHLANDER))
     {
	sprintf(buf,"Highlander Kills: %d  By Your Hand: %d\n\r",
	       ch->pcdata->highlander_data[ALL_KILLS],
               ch->pcdata->highlander_data[REAL_KILLS]);
	send_to_char(buf,ch);
     }

     sprintf( buf,
   "You are carrying %d.%d/%d.%d items, %d.%d of %d.%d pounds.\n\r",
   ch->carry_number / 10, ch->carry_number % 10,
   can_carry_n(ch) /10, can_carry_n(ch) % 10,
   get_carry_weight(ch) / 10, get_carry_weight(ch) % 10,
   can_carry_w(ch) / 10, can_carry_w(ch) % 10 );
     send_to_char( buf, ch );
/*     sprintf( buf,
   "You are carrying %d.%d/%d.%d items, %d%% weight capacity.\n\r",
   ch->carry_number / 10, ch->carry_number % 10,
   can_carry_n(ch) /10, can_carry_n(ch) % 10,
   (get_carry_weight(ch)*100)/can_carry_w(ch) );
     send_to_char( buf, ch );*/
 
     sprintf( buf,
"Str: %d/%d Int: %d/%d Wis: %d/%d "
"Dex: %d/%d Con: %d/%d Cha: %d/%d\n\r",
   ch->perm_stat[STAT_STR],      
   get_curr_stat(ch,STAT_STR),       
   ch->perm_stat[STAT_INT],
   get_curr_stat(ch,STAT_INT),
   ch->perm_stat[STAT_WIS],
   get_curr_stat(ch,STAT_WIS),
   ch->perm_stat[STAT_DEX],
   get_curr_stat(ch,STAT_DEX),
//   ch->perm_stat[STAT_AGT],
//   get_curr_stat(ch,STAT_AGT),
//   ch->perm_stat[STAT_END],         
//   get_curr_stat(ch,STAT_END),
   ch->perm_stat[STAT_CON],   
   get_curr_stat(ch,STAT_CON),
   ch->perm_stat[STAT_SOC],
   get_curr_stat(ch,STAT_SOC) );
     send_to_char( buf, ch );
 
     sprintf( buf,
   "You have %d exp, %ld gold, %ld silver.\n\r",
   ch->exp,  ch->gold, ch->silver );
     send_to_char( buf, ch );

     if ( ch->class == class_lookup("paladin") && !IS_NPC(ch) )
     {
	sprintf(buf,"Abolish Disease Status: %s (%d ticks)\n\r",
		 ch->pcdata->abolish_timer > 0 ? "unavailable" : "ready",
		ch->pcdata->abolish_timer );
	send_to_char(buf,ch);
    }

     /* RT shows exp to level */
     if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
     {
       sprintf (buf, 
   "You need %d exp to level.\n\r",
   ((ch->level + 1) * exp_per_level(ch,ch->pcdata->points) - ch->exp));
       send_to_char( buf, ch );
      }

	if(!IS_NPC(ch) && ch->pcdata->deity_favor_timer > 0)
 	{
 		if(ch->pcdata->quit_time > 0)
 			sprintf(buf, "%s's gaze is distant due to recent violence. (%d ticks)\n\r", deity_table[ch->pcdata->deity].pname, ch->pcdata->deity_favor_timer - 1);
 		else
			sprintf(buf, "The gaze of %s is on you. (%d ticks)\n\r", deity_table[ch->pcdata->deity].pname, ch->pcdata->deity_favor_timer - 1);
 		send_to_char(buf, ch);
 		if(ch->pcdata->deity_trial_timer > 0)
 		{
 			sprintf(buf, "Your trial is to slay a worthy non-player foe. (%d ticks)\n\r", ch->pcdata->deity_trial_timer - 1);
 			send_to_char(buf, ch);
 			switch(ch->pcdata->deity_trial)
 			{
 				case 0: send_to_char("If you flee you will fail this trial.\n\r", ch); break;
 				case 1: send_to_char("The foe must be good aligned.\n\r", ch); break;
 				case 2: send_to_char("The foe must be neutral aligned.\n\r", ch); break;
 				case 3: send_to_char("The foe must be evil aligned.\n\r", ch); break;
 				case 4: send_to_char("Your enemies resist your blows.\n\r", ch); break;
 				case 5: send_to_char("Your attack rate is greatly reduced.\n\r", ch); break;
 				case 6: send_to_char("Your damage and casting levels are greatly reduced.\n\r", ch); break;
 				case 7: send_to_char("You are vulnerable to your enemies' blows.\n\r", ch); break;
 				case 8: send_to_char("You can barely remember how to use your skills.\n\r", ch); break;
 				case 9: send_to_char("You can't avoid your enemies' blows.\n\r", ch); break;
 			}
 		}
 	}
 
     sprintf( buf, "Wimpy set to %d percent.\n\r", ch->wimpy );
     send_to_char( buf, ch );

     if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
	 send_to_char("You are drunk.\n\r",ch);

     if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_VAMP) )
     {
	if ( ch->pcdata->condition[COND_THIRST] == 0 ||
	     ch->pcdata->condition[COND_HUNGER] == 0 )
		 send_to_char("Your throat aches for blood.\n\r",ch);
     }
     else
     {
     if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] ==  0 )
   send_to_char( "You are thirsty.\n\r", ch );
     if ( !IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER]   ==  0 )
   send_to_char( "You are hungry.\n\r",  ch );
     }

     switch ( ch->position )
     {
     case POS_DEAD:     
   send_to_char( "You are DEAD!!\n\r",             ch );
   break;
     case POS_MORTAL:
   send_to_char( "You are mortally wounded.\n\r",  ch );
   break;
     case POS_INCAP:
   send_to_char( "You are incapacitated.\n\r",     ch );
   break;
     case POS_STUNNED:
   send_to_char( "You are stunned.\n\r",           ch );
   break;
     case POS_SLEEPING:
   send_to_char( "You are sleeping.\n\r",          ch );
   break;
     case POS_RESTING:
   send_to_char( "You are resting.\n\r",           ch );
   break;
     case POS_STANDING:
   send_to_char( "You are standing.\n\r",          ch );
   break;
     case POS_FIGHTING:
   send_to_char( "You are fighting.\n\r",          ch );
   break;
     }

     if ( is_mounted(ch) )
	 send_to_char("You are mounted.\n\r",ch);
     if ( ch->passenger != NULL )
	 send_to_char("You are carrying a passenger.\n\r",ch);

  strcpy(buf,"Saving Throw Adjustment: ");
  if ( ch->saving_throw < -18 )
  { strcat(buf,"godlike"); }
  else if(ch->saving_throw < -15)
  { strcat(buf,"excellent"); }
  else if(ch->saving_throw < -12)
  { strcat(buf,"good"); }
  else if(ch->saving_throw < -9)
  { strcat(buf,"fair"); }
  else if(ch->saving_throw < -6)
  { strcat(buf,"poor"); }
  else if(ch->saving_throw < -3)
  { strcat(buf,"horrible"); }
  else
  { strcat(buf,"non existent"); }

  if(ch->level >= 35)
  {
    send_to_char(buf,ch);
    sprintf(buf, " (%d)\n\r", ch->saving_throw);
    send_to_char(buf,ch);
  }
  else
  {
    strcat(buf, "\n\r");
    send_to_char(buf,ch);
  }

     if ( ch->species_enemy )
     {
  sprintf(buf,"Species Enemy: %s\n\r", race_table[ch->species_enemy].name);
  send_to_char(buf,ch);
     }

/* Enemy lists killed by NIGHTDAGGER on 04/25/2003 */
/*
     if (!IS_NPC(ch) && is_clan(ch))
     {
	if(IS_SET(ch->mhs,MHS_WARLOCK_ENEMY) || 
	   IS_SET(ch->mhs,MHS_ZEALOT_ENEMY) ||
	   IS_SET(ch->mhs,MHS_HONOR_ENEMY) ||
	   IS_SET(ch->mhs,MHS_POSSE_ENEMY))
	{
           sprintf(buf,"Enemy of: ");
	   if (IS_SET(ch->mhs,MHS_POSSE_ENEMY))
	      strcat(buf,"Posse ");
	   if (IS_SET(ch->mhs,MHS_WARLOCK_ENEMY))
	      strcat(buf,"Warlock ");
	   if (IS_SET(ch->mhs,MHS_ZEALOT_ENEMY))
	      strcat(buf,"Zealot ");
	   if (IS_SET(ch->mhs,MHS_HONOR_ENEMY))
	      strcat(buf,"Honor");
	   strcat(buf,"\n\r");
           send_to_char(buf,ch);
	}
     }
*/
     for (i = 0; i < 4; i++)
     {
   char * temp;
 
   switch(i)
   {
       case(AC_PIERCE):    temp = "piercing";      break;
       case(AC_BASH):      temp = "bashing";       break;
       case(AC_SLASH):     temp = "slashing";      break;
       case(AC_EXOTIC):    temp = "magic";         break;
       default:            temp = "error";         break;
   }

   send_to_char("You are ", ch);
 
   if      (GET_AC(ch,i) >=  101 ) 
       sprintf(buf,"hopelessly vulnerable to %s.",temp);
   else if (GET_AC(ch,i) >= 75) 
       sprintf(buf,"defenseless against %s.",temp);
   else if (GET_AC(ch,i) >= 50)
       sprintf(buf,"barely protected from %s.",temp);
   else if (GET_AC(ch,i) >= 25)
       sprintf(buf,"slightly armored against %s.",temp);
   else if (GET_AC(ch,i) >= 0)
       sprintf(buf,"somewhat armored against %s.",temp);
   else if (GET_AC(ch,i) >= -25)
       sprintf(buf,"armored against %s.",temp);
   else if (GET_AC(ch,i) >= -50)
       sprintf(buf,"well-armored against %s.",temp);
   else if (GET_AC(ch,i) >= -75)
       sprintf(buf,"very well-armored against %s.",temp);
   else if (GET_AC(ch,i) >= -100)
       sprintf(buf,"heavily armored against %s.",temp);
   else if (GET_AC(ch,i) >= -125)
       sprintf(buf,"superbly armored against %s.",temp);
   else if (GET_AC(ch,i) >= -150)
       sprintf(buf,"almost invulnerable to %s.",temp);
   else
       sprintf(buf,"divinely armored against %s.",temp);
 
   if(ch->level >= 25)
   {
     send_to_char(buf,ch);
     sprintf(buf, " (%d)\n\r", GET_AC(ch,i));
     send_to_char(buf,ch);
   }
   else
   {
     strcat(buf, "\n\r");
     send_to_char(buf,ch);
   }
     }
 
 
     /* RT wizinvis and holy light */
     if ( IS_IMMORTAL(ch))
     {
       send_to_char("Holy Light: ",ch);
       if (IS_SET(ch->act,PLR_HOLYLIGHT))
   send_to_char("on",ch);
       else
   send_to_char("off",ch);
  
       if (ch->invis_level)
       {
   sprintf( buf, "  Invisible: level %d",ch->invis_level);
   send_to_char(buf,ch);
       }
 
       if (ch->incog_level)
       {
   sprintf(buf,"  Incognito: level %d",ch->incog_level);
   send_to_char(buf,ch);
       }
       send_to_char("\n\r",ch);
     }
 
     if ( ch->level >= 15 )
     {
	if (get_eq_char(ch,WEAR_SECOND) == NULL)
           sprintf( buf, "Hitroll: %d  Damroll: %d.\n\r",
                   GET_HITROLL(ch), GET_DAMROLL(ch) );
	else
           sprintf( buf, "Hitroll: %d/%d  Damroll: %d/%d.\n\r",
                   GET_HITROLL(ch),GET_SECOND_HITROLL(ch), 
		   GET_DAMROLL(ch),GET_SECOND_DAMROLL(ch) );
        send_to_char( buf, ch );
     }
    
     send_to_char( "You are ", ch );
    if ( ch->alignment >  900 ) send_to_char( "an angelic ",ch);
     else if ( ch->alignment >  700 ) send_to_char( "a saintly ",ch );
     else if ( ch->alignment >  350 ) send_to_char( "a good ",ch );
     else if ( ch->alignment >  150 ) send_to_char( "a kind ",ch );
     else if ( ch->alignment > -150 ) send_to_char( "a neutral ", ch );
     else if ( ch->alignment > -350 ) send_to_char( "a mean ", ch );
     else if ( ch->alignment > -700 ) send_to_char( "an evil ", ch );
     else if ( ch->alignment > -900 ) send_to_char( "a demonic ", ch );
     else                             send_to_char( "a satanic ", ch );


     sprintf(buf,"follower of %s.  ", IS_NPC(ch) ? "yourself." :
	deity_table[ch->pcdata->deity].name );
     send_to_char(buf,ch);

   sprintf( buf, "Sac Points: %d\n\r", IS_NPC(ch) ? 0 : ch->pcdata->sac );
   send_to_char( buf, ch );
 
     if ( ch->class == class_lookup("elementalist"))
     {
        if (ch->pcdata->node != 0)
        {
	   location = get_room_index(ch->pcdata->node);
           sprintf( buf, "Node Location: %s in %s\n\r",
              location->name,location->area->name );
           send_to_char( buf, ch );
        }
     }

     if (IS_SET(ch->mhs,MHS_SHAPEMORPHED))
     {
	sprintf( buf, "You are morphed into: %s\n\r",
	   ch->long_descr );
	send_to_char( buf,ch );
     }

     if (IS_SET(ch->display,DISP_SHOW_AFFECTS))
   do_affects(ch,"");
 }
 
 void do_affects(CHAR_DATA *ch, char *argument )
 {
     AFFECT_DATA *paf, *paf_last = NULL;
     char buf[MAX_STRING_LENGTH];
     
     if ( ch->affected != NULL )
     {
   send_to_char( "You are affected by the following spells:\n\r", ch );
   for ( paf = ch->affected; paf != NULL; paf = paf->next )
   {
       if (paf_last != NULL && paf->type == paf_last->type)
     if (ch->level >= 20)
         sprintf( buf, "                      ");
     else
         continue;
       else
     sprintf( buf, "Spell: %-15s", skill_table[paf->type].name );
 
       send_to_char( buf, ch );
 
       if ( ch->level >= 20 )
       {
     if ( paf->where == DAMAGE_OVER_TIME )
     {
	if ( paf->modifier < 0 || paf->location < 0 )
	 sprintf( buf, ": restoration ");
	else
         sprintf( buf, ": damage over time ");
     }
     else
     sprintf( buf,
         ": modifies %s by %d ",
         affect_loc_name( paf->location ),
         paf->modifier);
     send_to_char( buf, ch );
     if ( paf->duration == -1 )
         sprintf( buf, "permanently" );
     else	/* 10 DOT pulses per tick */
         sprintf( buf, "for %d hours", (paf->where == DAMAGE_OVER_TIME) ? paf->duration / 10 : paf->duration );
     send_to_char( buf, ch );
       }
 
       send_to_char( "\n\r", ch );
       paf_last = paf;
   }
     }
     else 
   send_to_char("You are not affected by any spells.\n\r",ch);
 
     return;
 }
 
 
 
 char *  const   day_name        [] =
 {
     "the Moon", "the Bull", "Deception", "Thunder", "Freedom",
     "the Great Gods", "the Sun"
 };
 
 char *  const   month_name      [] =
 {
     "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
     "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
     "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
     "the Long Shadows", "the Ancient Darkness", "the Great Evil"
 };
 
 void do_time( CHAR_DATA *ch, char *argument )
 {
     extern char str_boot_time[];
     char buf[MAX_STRING_LENGTH];
     char *suf;
     int day;
 
     day     = time_info.day + 1;
 
    if ( day > 4 && day <  20 ) suf = "th";
     else if ( day % 10 ==  1       ) suf = "st";
     else if ( day % 10 ==  2       ) suf = "nd";
     else if ( day % 10 ==  3       ) suf = "rd";
     else                             suf = "th";
 
     sprintf( buf,
   "It is %d o'clock %s, Day of %s, %d%s the Month of %s.\n\r",
   (time_info.hour % 12 == 0) ? 12 : time_info.hour %12,
   time_info.hour >= 12 ? "pm" : "am",
   day_name[day % 7],
   day, suf,
   month_name[time_info.month]);
     send_to_char(buf,ch);
     sprintf(buf,"MHS started up at %s\n\rThe system time is %s.\n\r",
   str_boot_time,
   (char *) ctime( &current_time )
   );
 
     send_to_char( buf, ch );

    // if (override)
    //     send_to_char("Double experience is currently ON.\n\r",ch);
     if (override)
     {
       if(override < 0)
         send_to_char("Double experience is currently ON.\n\r",ch);
       else
       {
         if(override < 90)// Less than an hour
           send_to_char("Double experience is currently ON. Less than an hour remains.\n\r",ch);
         else if(override < 135)// Hour and a half
           send_to_char("Double experience is currently ON. About 1 hour remains.\n\r", ch);
         else
         {
           sprintf(buf, "Double experience is currently ON. About %d hours remain.\n\r", (override + 45) / 90);
           send_to_char(buf, ch);
         }
       }
     }

     return;
 }
 
 
 
 void do_weather( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
 
     static char * const sky_look[4] =
     {
   "cloudless",
   "cloudy",
   "rainy",
   "lit by flashes of lightning"
     };
 
     if ( !IS_OUTSIDE(ch) )
     {
   send_to_char( "You can't see the weather indoors.\n\r", ch );
   return;
     }
 
     sprintf( buf, "The sky is %s and %s.\n\r",
   sky_look[weather_info.sky],
   weather_info.change >= 0
   ? "a warm southerly breeze blows"
   : "a cold northern gust blows"
   );
     send_to_char( buf, ch );
     return;
 }
 
 
 
 void do_help( CHAR_DATA *ch, char *argument )
 {
     HELP_DATA *pHelp;
     char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];
     int col;
      
     col = 0;
    
     if ( argument[0] == '\0' )
        argument = "summary";
 
     /* this parts handles help a b so that it returns help 'a b' */
     argall[0] = '\0';
     while (argument[0] != '\0' )
     {
        argument = one_argument(argument,argone);
        if (argall[0] != '\0')
           strcat(argall," ");
        strcat(argall,argone);
     }
 
     for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
     {
        if ( pHelp->level > get_trust( ch ) )
           continue;
 
        if ( is_name( argall, pHelp->keyword ) )
        {
           if ( pHelp->level >= 0 && str_cmp( argall, "imotd" ) )
           {
              send_to_char( pHelp->keyword, ch );
              send_to_char( "\n\r", ch );
           }
 
       /*
        * Strip leading '.' to allow initial blanks.
        */
           if ( pHelp->text[0] == '.' )
              page_to_char( pHelp->text+1, ch );
           else
              page_to_char( pHelp->text  , ch );
           return;
        }
     }
 
     send_to_char (argall,ch);
     send_to_char ("\n\r",ch);
/*
     if (str_cmp(capitalize(argall),"topics") && (!str_prefix("topics",argall)
        || !str_suffix("topics",argall)))
*/
     if (!str_suffix("topics",argall))
     {
        for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
        {
           send_to_char( pHelp->keyword, ch );
           send_to_char( ", ", ch);

           if (++col % 6 == 0)
              send_to_char("\n\r",ch);
        }
 
        if ( col % 6 != 0)
           send_to_char("\n\r",ch);
        return;
     }

     send_to_char( "No help on that word.\n\r", ch );
     return;
 }
 
 
/* whois command */
void do_whois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH], cbuf[MAX_STRING_LENGTH], sbuf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;    
    char wizi[50],  incog[50];
    char surname[15];

    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
  send_to_char("You must provide a name.\n\r",ch);
  return;
    }

    output = new_buf();

    for (d = descriptor_list; d != NULL; d = d->next)
    {
  CHAR_DATA *wch;
  char const *class;
  char who_name[200];
  int len;
  char const *display_race;

  if (d->connected != CON_PLAYING || !can_see(ch,d->character,TRUE))
      continue;
  
  wch = ( d->original != NULL ) ? d->original : d->character;

  if (!can_see(ch,wch,TRUE))
      continue;

    cbuf[0] = '\0';
    sbuf[0] = '\0';
    if (!clan_table[wch->clan].independent &&
         clan_table[wch->clan].true_clan) 
       sprintf(sbuf, "/%d] ", wch->pcdata->rank);

    if (clan_table[wch->clan].independent 
	&& clan_table[wch->clan].true_clan) 
       sprintf(sbuf, "] ");

    if ( wch->clan && !is_clan(wch)) 
       sprintf(sbuf," ");

    sprintf(cbuf,clan_table[wch->clan].who_name);
    if ( clan_table[wch->clan].hidden && 
	( !is_same_clan(wch,ch) || (IS_IMMORTAL(ch) && get_trust(ch)<58)) )
	{
	sprintf(cbuf,"[ Loner ] ");
	sbuf[0] = '\0';
	}

  if (!str_prefix(arg,wch->name))
  {
      found = TRUE;
      
      /* work out the printing */
          class = class_table[wch->class].who_name;
      switch(wch->level)
      {
    case MAX_LEVEL - 0 : class = "IMPLEMENTOR ";   break;
    case MAX_LEVEL - 1 : class = "  CREATOR   "; break;
    case MAX_LEVEL - 2 : class = " SUPREMACY  "; break;
    case MAX_LEVEL - 3 : class = "   DEITY    "; break;
    case MAX_LEVEL - 4 : class = "    GOD     "; break;
    case MAX_LEVEL - 5 : class = "  IMMORTAL  "; break;
    case MAX_LEVEL - 6 : class = "  DEMIGOD   "; break;
    case MAX_LEVEL - 7 : class = "   ANGEL    "; break;
    case MAX_LEVEL - 8 : class = "   AVATAR   "; break;
      }

     /*
      * Figure out what to print for race.
      */
     if (IS_SET(wch->mhs,MHS_SHAPESHIFTED))
     {
        display_race = pc_race_table[wch->save_race].who_name;
     }
     else
     {
        display_race = pc_race_table[wch->race].who_name; 
     }
      
    surname[0] = '\0';
    if ( wch->pcdata->surname != NULL )
    	sprintf(surname," %s",wch->pcdata->surname);

      /* a little formatting */
  if (wch->level >= MAX_LEVEL - 8) {  
    if ( wch->pcdata->who_name && (wch->pcdata->who_name[0] != '\0')) {
      strcpy (who_name,"             ");
      len = strlen (wch->pcdata->who_name); 
      if (len > 11) {
        strncpy (who_name,wch->pcdata->who_name,12);
      } else {        
        strncpy (&who_name[(12-len)/2],wch->pcdata->who_name,len);        
      }
      who_name[12] = '\0';
      class = who_name;     
    }

    IS_SET(ch->display, DISP_BRIEF_WHOLIST) ?
    sprintf(incog,"(I%d) ", wch->incog_level) :
    sprintf(incog,"({WIncog{x@{W%d{x) ", wch->incog_level);
    
    IS_SET(ch->display, DISP_BRIEF_WHOLIST) ?
    sprintf(wizi,"(W%d) ", wch->invis_level) :
    sprintf(wizi,"({WWizi{x@{W%d{x) ", wch->invis_level);

    sprintf(buf, "[%s] %s%s%s%s%s%s%s%s%s%s%s%s\n\r{x",
     class,
           wch->incog_level >= LEVEL_HERO ? incog : "",
           wch->invis_level >= LEVEL_HERO ? wizi   : "",
           cbuf[0] != '\0' ? cbuf : "",
	   sbuf[0] != '\0' ? sbuf : "",
           IS_SET(wch->comm, COMM_AFK) ? "[{CAFK{x] " : 
		(IS_SET(wch->pcdata->clan_flags, CLAN_PAB) ? "[{WPAB{x] " : ""),
           IS_SET(wch->comm, COMM_IN_OLC) ? "[OLC] " : "",
           IS_SET(wch->act, PLR_DWEEB) ? "(DWEEB) " : "",
	wch->trumps > 0 ?
           (IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : "({rTh{x) ") : 
		(IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : 
			(IS_SET(wch->wiznet,PLR_RUFFIAN) ? "({rR{x) ":"") ),
           IS_SET(wch->act,PLR_THIEF) ? "({RTf{x) " : "",
              wch->name, 
	 	IS_SET(ch->display,DISP_SURNAME) ? surname : "",
	      IS_NPC(wch) ? "" : (IS_SET(ch->display,DISP_NOTITLES) ? "" : wch->pcdata->title) );
          add_buf(output,buf);
  }
  else
  {
      sprintf(buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r{x",
       wch->level, display_race, class,
       wch->incog_level >= LEVEL_HERO ? "(I) ": "",
       wch->invis_level >= LEVEL_HERO ? "(W) " : "",
       cbuf[0] != '\0' ? cbuf : "",
       sbuf[0] != '\0' ? sbuf : "",
       IS_SET(wch->comm, COMM_AFK) ? "[{CAFK{x] " : 
		(IS_SET(wch->pcdata->clan_flags, CLAN_PAB) ? "[{WPAB{x] " : ""),
             IS_SET(wch->act, PLR_DWEEB) ? "(DWEEB) " : "",
	wch->trumps > 0 ?
           (IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : "({rTh{x) ") :
		(IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : 
			(IS_SET(wch->wiznet,PLR_RUFFIAN) ? "({rR{x) ":"") ),
             IS_SET(wch->act,PLR_THIEF) ? "({RTf{x) " : "",
    wch->name, 
    IS_SET(ch->display,DISP_SURNAME) ? surname : "", IS_NPC(wch) ? "" : (IS_SET(ch->display,DISP_NOTITLES) ? "" : wch->pcdata->title) );
      add_buf(output,buf);
  }
  }
    }

    if (!found)
    {
  send_to_char("No one of that name is playing.\n\r",ch);
  return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}

void do_cstat(CHAR_DATA *ch)
{
    CSTAT_DATA *cstat;
    char buf[MAX_STRING_LENGTH];
    
    send_to_char("Are stats really that important?  Go PKill now.",ch);
    return;


    send_to_char("Clan    Kills\n\r",ch);
    send_to_char("----    -----\n\r",ch);
    sprintf(buf,"Avarice %d\n\r",avarice_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Demise  %d\n\r",demise_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Honor   %d (%d Demise Vanquished)\n\r",
            honor_kills,honor_demise_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Posse   %d (%d Killers %d Thugs %d Ruffians %d Thieves\n\r",
            posse_kills,posse_killer_kills,posse_thug_kills,
            posse_ruffian_kills,posse_thief_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Warlock %d\n\r",warlock_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Zealot  %d\n\r",zealot_kills);
    send_to_char(buf,ch);
/*
    sprintf(buf,"Honor   %d\n\r",honor_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Posse   %d\n\r",posse_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Warlock %d\n\r",warlock_kills);
    send_to_char(buf,ch);
    sprintf(buf,"Zealot  %d\n\r",zealot_kills);
    send_to_char(buf,ch);
*/
/*
    for (cstat = cstat_first;cstat != NULL;cstat = cstat->next)
    {
       if (cstat->kills > 99)
sprintf(buf,"%d    %s\n\r",cstat->kills,clan_table[cstat->clan].name);
       if (cstat->kills > 9)
sprintf(buf,"%d     %s\n\r",cstat->kills,clan_table[cstat->clan].name);
       if (cstat->kills < 10)
sprintf(buf,"%d      %s\n\r",cstat->kills,clan_table[cstat->clan].name);
       send_to_char(buf,ch);
    }
*/
    return;
}

/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char sbuf[MAX_STRING_LENGTH], cbuf[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int level;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fNonClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    bool shownCDR = FALSE;
    char incog[50], wizi[50];
    bool fHighlander = FALSE;
    bool fGladiator = FALSE;
    char surname[15];

    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = MAX_LEVEL;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
  rgfClan[iClan] = FALSE;
 
    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];
 
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;
 
        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
            case 1: iLevelLower = atoi( arg ); break;
            case 2: iLevelUpper = atoi( arg ); break;
            default:
                send_to_char( "Only two level numbers allowed.\n\r", ch );
                return;
            }
        }
        else
        {
 
            /*
             * Look for classes to turn on.
             */
            if ( arg[0] == 'i' )
            {
                fImmortalOnly = TRUE;
            }
            else
            {
                iClass = class_lookup(arg);
                if (iClass == -1)
                {
                    iRace = race_lookup(arg);
                    if (iRace == 0 || iRace >= MAX_PC_RACE)
                    {
		       if (!str_prefix(arg,"gladiator"))
			  fGladiator = TRUE;
		       else
		       {
                          if (!str_prefix(arg,"clan"))
                             fClan = TRUE;
                          else
                          {
			     if (!str_prefix(arg,"nonclan"))
			     {
				fNonClan = TRUE;
			     }
			     else
			     {
                                iClan = clan_lookup(arg);
                                if  (iClan)
                                {
                                   fClanRestrict = TRUE;
                                   rgfClan[iClan] = TRUE;
                                }
                                else
                                {
                                   if (!str_prefix(arg,"highlander"))
                                      fHighlander = TRUE;
                                   else
                                   {
			              send_to_char("That's not a valid race, class or clan.\n\r",ch);
                                      return;
				   }
                                } 
                             }
                          }
                       }
		    }
                    else
                    {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                }
                else
                {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }
    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();

  for ( level = MAX_LEVEL+1; level > 0; level-- )
   {
    if( level == MAX_LEVEL - 1 && !fClan )
	{
	sprintf(buf,"\n\rIMMORTALS\n\r---------\n\r");
	add_buf(output,buf);
	}
    if( level == MAX_LEVEL - 9 )
	{
	if(fImmortalOnly) 
	  {
	   level = 0;
	   continue;
	  }
	else
	{
	sprintf(buf,"\n\rMORTALS\n\r-------\n\r");
	add_buf(output,buf);
	}
	}

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch;
        char const *class;
        char who_name[200];
        int len;
	char const *display_race;
 
        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( d->connected != CON_PLAYING || !can_see( ch, d->character, TRUE) )
            continue;
 
        wch   = ( d->original != NULL ) ? d->original : d->character;

  if (!can_see(ch,wch,TRUE) 
           || (wch->level == level && IS_SET(wch->display,DISP_CODER))
      || (wch->level != level 
      && !(wch->level == level-1 && IS_SET(wch->display,DISP_CODER))))
      continue;

        if ( wch->level < iLevelLower
        ||   wch->level > iLevelUpper
        || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
        || ( fClassRestrict && !rgfClass[wch->class] )
        || ( fRaceRestrict && !rgfRace[wch->race])
  || ( fClan && !is_clan(wch))
  || ( fNonClan && is_clan(wch))
  || ( fHighlander && !IS_SET(wch->mhs,MHS_HIGHLANDER))
  || ( fGladiator && !IS_SET(wch->mhs,MHS_GLADIATOR))
  || ( fClanRestrict && !rgfClan[wch->clan] )
  || ( fClanRestrict && rgfClan[wch->clan] && clan_table[wch->clan].hidden
	&& !is_same_clan(wch,ch))
	 )
            continue;

	if ( fClanRestrict && rgfClan[wch->clan] &&
		clan_table[wch->clan].hidden )
			continue;

        nMatch++;
 
    if( level == MAX_LEVEL 
      && ((wch->level == level-1 && IS_SET(wch->display,DISP_CODER))
	  || wch->level == level)
      && nMatch > 0 && !fClan && !shownCDR )
	{
	sprintf(buf,"CODERS\n\r------\n\r");
	add_buf(output,buf);
	shownCDR = TRUE;
	}

        /*
         * Figure out what to print for class.
   */
  class = class_table[wch->class].who_name; 

  switch ( wch->level )
  {
  default: break;
            {
                case MAX_LEVEL - 0 : class = "IMPLEMENTOR ";    break;
                case MAX_LEVEL - 1 : class = "  CREATOR   ";    break;
                case MAX_LEVEL - 2 : class = " SUPREMACY  ";    break;
                case MAX_LEVEL - 3 : class = "   DEITY    ";    break;
                case MAX_LEVEL - 4 : class = "    GOD     ";    break;
                case MAX_LEVEL - 5 : class = "  IMMORTAL  ";    break;
                case MAX_LEVEL - 6 : class = "  DEMIGOD   ";    break;
                case MAX_LEVEL - 7 : class = "   ANGEL    ";    break;
                case MAX_LEVEL - 8 : class = "  AVATAR    ";    break;
            }
  }

    /*
     * Figure out what to print for race.
     */
 
   if (IS_SET(wch->mhs,MHS_SHAPESHIFTED))
    {
       display_race = pc_race_table[wch->save_race].who_name;
    }
    else
    {
       display_race = pc_race_table[wch->race].who_name; 
    }



    sbuf[0] = '\0';
    if (!clan_table[wch->clan].independent &&
	is_clan(wch)) sprintf(sbuf, "/%d] ", wch->pcdata->rank);
    if (clan_table[wch->clan].independent 
	&& is_clan(wch)) sprintf(sbuf, "] ");
    if (wch->clan && !is_clan(wch) ) sprintf(sbuf," ");

    sprintf(cbuf,clan_table[wch->clan].who_name);
    if(clan_table[wch->clan].hidden &&
       ( !is_same_clan(wch,ch) || (IS_IMMORTAL(ch) && get_trust(ch)<58) ) )
        {
	sprintf(cbuf,"[ Loner ] ");
	sbuf[0] = '\0';
	}

    if (IS_SET(wch->mhs,MHS_GLADIATOR) && gladiator_info.started)
    {
       if(wch->pcdata->gladiator_team == 2)
         sprintf(cbuf,"[ Barbarian ] ");
       else
         sprintf(cbuf,"[ Gladiator ] ");
       sbuf[0] = '\0';
    }

    surname[0] = '\0';
    if ( wch->pcdata->surname != NULL )
	sprintf(surname," %s",wch->pcdata->surname);

  /*
   * Format it up.
   */

  if (wch->level >= MAX_LEVEL - 8)
        {

    if ( wch->pcdata->who_name && (wch->pcdata->who_name[0] != '\0')) {
      strcpy (who_name,"             ");
      len = strlen (wch->pcdata->who_name); 
      if (len > 11) {
        strncpy (who_name,wch->pcdata->who_name,12);
      } else {        
        strncpy (&who_name[(12-len)/2],wch->pcdata->who_name,len);        
      }
      who_name[12] = '\0';
      class = who_name;
    }


    IS_SET(ch->display, DISP_BRIEF_WHOLIST) ?
    sprintf(incog,"(I%d) ", wch->incog_level ) :
    sprintf(incog,"({WIncog{x@{W%d{x) ", wch->incog_level );

    IS_SET(ch->display, DISP_BRIEF_WHOLIST) ?
    sprintf(wizi,"(W%d) ", wch->invis_level ) :
    sprintf(wizi,"({WWizi{x@{W%d{x) ", wch->invis_level );

    sprintf(buf,  "[%s] %s%s%s%s%s%s%s%s%s%s%s%s\n\r{x",
     class,
           wch->incog_level >= LEVEL_HERO ? incog : "",
           wch->invis_level >= LEVEL_HERO ? wizi   : "",
   	   cbuf[0] != '\0' ? cbuf : "",
	   sbuf[0] != '\0' ? sbuf : "",
             IS_SET(wch->comm, COMM_AFK) ? "[{CAFK{x] " : 
		(IS_SET(wch->pcdata->clan_flags, CLAN_PAB) ? "[{WPAB{x] " : ""),
             IS_SET(wch->comm, COMM_IN_OLC) ? "[OLC] " : "",
             IS_SET(wch->act, PLR_DWEEB) ? "(DWEEB) " : "",
	wch->trumps > 0 ?
           (IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : "({rTh{x) ") :
		(IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : 
			(IS_SET(wch->wiznet,PLR_RUFFIAN) ? "({rR{x) ":"") ),
             IS_SET(wch->act,PLR_THIEF) ? "({RTf{x) " : "",
             wch->name, 
	     IS_SET(ch->display,DISP_SURNAME) ? surname : "",
             IS_NPC(wch) ? "" : (IS_SET(ch->display,DISP_NOTITLES) ? "" : wch->pcdata->title) );
            add_buf(output,buf);
        }
        else
  {
  sprintf( buf, "[%2d %6s %s]{x%s%s%s%s%s%s%s%s%s%s%s%s\n\r{x",
      wch->level, 
      display_race, 
      class,
      (IS_SET(wch->act,PLR_CANCLAN) && wch->level < 25 &&
       !is_clan(wch) && IS_IMMORTAL(ch ) ) ? "*" : " ",
      wch->incog_level >= LEVEL_HERO ? "(I) " : "",
      wch->invis_level >= LEVEL_HERO ? "(W) " : "",
      cbuf[0] != '\0' ? cbuf : "",
      sbuf[0] != '\0' ? sbuf : "",
      IS_SET(wch->comm, COMM_AFK) ? "[{CAFK{x] " : 
		(IS_SET(wch->pcdata->clan_flags, CLAN_PAB) ? "[{WPAB{x] " : ""),
            IS_SET(wch->act, PLR_DWEEB) ? "(DWEEB) " : "",
	wch->trumps > 0 ?
           (IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : "({rTh{x) ") :
		(IS_SET(wch->act,PLR_KILLER) ? "({RK{x) " : 
			(IS_SET(wch->wiznet,PLR_RUFFIAN) ? "({rR{x) ":"") ),
            IS_SET(wch->act, PLR_THIEF)  ? "({RTf{x) "  : "",
      wch->name,
      IS_SET(ch->display,DISP_SURNAME) ? surname : "",
      IS_NPC(wch) ? "" : (IS_SET(ch->display,DISP_NOTITLES) ? "" : wch->pcdata->title) );
  add_buf(output,buf);
  }
    }
  }

    sprintf( buf2, "\n\rPlayers found: %d\n\r", nMatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}

 void do_count ( CHAR_DATA *ch, char *argument )
 {
     int count;
     DESCRIPTOR_DATA *d;
     char buf[MAX_STRING_LENGTH];
 
     count = 0;
 
     for ( d = descriptor_list; d != NULL; d = d->next )
   if ( d->connected == CON_PLAYING && can_see( ch, d->character, TRUE ) )
       count++;
 
     max_on = UMAX(count,max_on);
 
     if (max_on == count)
   sprintf(buf,"There are %d characters on, the most so far today.\n\r",
       count);
     else
   sprintf(buf,"There are %d characters on, the most on today was %d.\n\r",
       count,max_on);
 
     send_to_char(buf,ch);
 }
 
 void do_inventory( CHAR_DATA *ch, char *argument )
 {
     send_to_char( "You are carrying:\n\r", ch );
     show_list_to_char( ch->carrying, ch, TRUE, TRUE, FALSE );
     return;
 }
 
 
 
 void do_equipment( CHAR_DATA *ch, char *argument )
 {
     OBJ_DATA *obj;
     int iWear;
     bool found;
 
     send_to_char( "You are using:\n\r", ch );
     found = FALSE;
     for ( iWear = 0; iWear < MAX_WEAR; iWear++ )
     {
   if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
     {
	if(!IS_NPC(ch) && !IS_SET(ch->display, DISP_BRIEF_EQLIST) )
	 {
	  send_to_char( where_name[iWear], ch );
	  send_to_char( "- - -\n\r", ch );
	 }
       continue;
     }
 
   send_to_char( where_name[iWear], ch );

   if ( can_see_obj( ch, obj ) )
   {
       send_to_char( format_obj_to_char( obj, ch, TRUE ), ch );
       send_to_char( "\n\r", ch );
   }
   else
   {
       send_to_char( "something.\n\r", ch );
   }
   found = TRUE;
     }
 
     if ( !found )
   send_to_char( "Nothing.\n\r", ch );
 
     return;
 }
 
 
 
 void do_compare( CHAR_DATA *ch, char *argument )
 {
     char arg1[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     OBJ_DATA *obj1;
     OBJ_DATA *obj2;
     int value1;
     int value2;
     char *msg;
 
     argument = one_argument( argument, arg1 );
     argument = one_argument( argument, arg2 );
     if ( arg1[0] == '\0' )
     {
   send_to_char( "Compare what to what?\n\r", ch );
   return;
     }
 
     if ( ( obj1 = get_obj_carry( ch, arg1 ) ) == NULL )
     {
   send_to_char( "You do not have that item.\n\r", ch );
   return;
     }
 
     if (arg2[0] == '\0')
     {
   for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
   {
       if (obj2->wear_loc != WEAR_NONE
       &&  can_see_obj(ch,obj2)
       &&  obj1->item_type == obj2->item_type
       &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
     break;
   }
 
   if (obj2 == NULL)
   {
       send_to_char("You aren't wearing anything comparable.\n\r",ch);
       return;
   }
     } 
 
     else if ( (obj2 = get_obj_carry(ch,arg2) ) == NULL )
     {
   send_to_char("You do not have that item.\n\r",ch);
   return;
     }
 
     msg         = NULL;
     value1      = 0;
     value2      = 0;
 
     if ( obj1 == obj2 )
     {
   msg = "You compare $p to itself.  It looks about the same.";
     }
     else if ( obj1->item_type != obj2->item_type )
     {
   msg = "You can't compare $p and $P.";
     }
     else
     {
   switch ( obj1->item_type )
   {
   default:
       msg = "You can't compare $p and $P.";
       break;
 
   case ITEM_ARMOR:
       value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
       value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
       break;
 
   case ITEM_WEAPON:
       if (obj1->pIndexData->new_format)
     value1 = (1 + obj1->value[2]) * obj1->value[1];
       else
     value1 = obj1->value[1] + obj1->value[2];
 
       if (obj2->pIndexData->new_format)
     value2 = (1 + obj2->value[2]) * obj2->value[1];
       else
     value2 = obj2->value[1] + obj2->value[2];
       break;
   }
     }
 
     if ( msg == NULL )
     {
        if ( value1 == value2 ) msg = "$p and $P look about the same.";
   else if ( value1  > value2 ) msg = "$p looks better than $P.";
   else                         msg = "$p looks worse than $P.";
     }
 
     act( msg, ch, obj1, obj2, TO_CHAR,FALSE );
     return;
 }
 
 
 
 void do_credits( CHAR_DATA *ch, char *argument )
 {
     do_help( ch, "diku" );
     return;
 }
 
 
 
 void do_where( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
     char arg[MAX_INPUT_LENGTH];
     CHAR_DATA *victim;
     DESCRIPTOR_DATA *d;
     ROOM_INDEX_DATA *scan_room;
     bool found;

   if(!IS_NPC(ch) && ch->move < 4)
   {
      send_to_char("You can't get up enough energy.\n\r",ch);
      return;
   }

   if(ch->in_room->sector_type == SECT_OBS_ROOM)
	scan_room = get_room_index(ch->in_room->obs_target);
   else
	scan_room = ch->in_room;

   ch->move = UMAX(ch->move - apply_chi(ch,4), 0);

   one_argument( argument, arg );
  
   if ( arg[0] == '\0' )
   {
      send_to_char( "Players near you:\n\r", ch );
      found = FALSE;
      for ( d = descriptor_list; d; d = d->next )
      {
         if ( (d->connected == CON_PLAYING)
         && ( (victim = d->character) != NULL )
         && ( !IS_NPC(victim) )
         && ( victim->in_room != NULL )
         && ( !IS_SET(victim->in_room->room_flags,ROOM_NOWHERE) )
         && ( (is_room_owner(ch,victim->in_room) )
         ||   (!room_is_private(ch,victim->in_room)) )
         && ( victim->in_room->area == scan_room->area )
         && ( can_see( ch, victim,FALSE)) 
         && ( !is_clan( ch ) || ( is_clan(ch) && !IS_AFFECTED(ch,AFF_BLIND)))
         && ( check_match(ch, victim) )
         && (get_skill(ch,gsn_scan) >= 
	    (number_percent() + (victim->level - ch->level)*2 ))) 

         {
            if(found == FALSE)
	    {
	       found = TRUE;
	       check_improve(ch,gsn_scan,TRUE,4);
	    }
	    if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE && victim != ch)
               sprintf( buf, "%-28s %s\n\r",
               victim->long_descr, victim->in_room->name );
	    else
               sprintf( buf, "%-28s %s\n\r",
               victim->name, victim->in_room->name );
            send_to_char( buf, ch );
         }
      }
      if ( !found )
      {
         send_to_char( "None\n\r", ch );
         check_improve(ch,gsn_scan,FALSE,7);
      }
   }
   else
   {
      found = FALSE;
      for ( victim = char_list; victim != NULL; victim = victim->next )
      {
         if ( victim->in_room != NULL
         &&   victim->in_room->area == scan_room->area
         &&   !IS_AFFECTED(victim, AFF_HIDE)
         &&   !IS_AFFECTED(victim, AFF_SNEAK)
         &&   !IS_SET(victim->in_room->room_flags,ROOM_NOWHERE) 
         &&   can_see( ch, victim, FALSE )
         &&   is_name( arg, victim->name ) 
	 && (get_skill(ch,gsn_scan) >=
            (number_percent() + victim->level - ch->level)))

         {
           if(found == FALSE)
	   {
	     found = TRUE;
	     check_improve(ch,gsn_scan,TRUE,4);
	   }
	   if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE)
           {
                found = FALSE;
	        break;// No scanning specific names in blind gladiators
//              sprintf( buf, "%-28s %s\n\r",
//              victim->long_descr, victim->in_room->name );
           }
	   else
              sprintf( buf, "%-28s %s\n\r",
              PERS(victim, ch, FALSE), victim->in_room->name );
           send_to_char( buf, ch );
           break;
        }
     }
     if ( !found )
     {
       act( "You didn't find any $T.", ch, NULL, arg, TO_CHAR,FALSE );
       check_improve(ch,gsn_scan,FALSE,6);
     }
  }
 
  return;
}
 
 
 
 
 void do_consider( CHAR_DATA *ch, char *argument )
 {
     char arg[MAX_INPUT_LENGTH];
     CHAR_DATA *victim;
     char *msg;
     int diff;
 
     one_argument( argument, arg );
 
     if ( arg[0] == '\0' )
     {
   send_to_char( "Consider killing whom?\n\r", ch );
   return;
     }
 
     if ( ( victim = get_char_room( ch, arg ) ) == NULL )
     {
   send_to_char( "They're not here.\n\r", ch );
   return;
     }
 
     if (is_safe(ch,victim))
     {
   send_to_char("Don't even think about it.\n\r",ch);
   return;
     }
 
     diff = victim->level - ch->level;
 
   if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] >6)
   {
      diff = number_range(1,60) - ch->level;
      if (ch->race == race_lookup("dwarf"))
      {
         diff -= 30;
      }
   }
 
    if ( diff <= -10 ) msg = "You can kill $N naked and weaponless.";
     else if ( diff <=  -5 ) msg = "$N is no match for you.";
     else if ( diff <=  -2 ) msg = "$N looks like an easy kill.";
     else if ( diff <=   1 ) msg = "The perfect match!";
     else if ( diff <=   4 ) msg = "$N says 'Do you feel lucky, punk?'.";
     else if ( diff <=   9 ) msg = "$N laughs at you mercilessly.";
     else                    msg = "Death will thank you for your gift.";
 
     act( msg, ch, NULL, victim, TO_CHAR,FALSE );
     return;
 }
 
 
 
 void set_title( CHAR_DATA *ch, char *title )
 {
     char buf[MAX_STRING_LENGTH];
 
     if ( IS_NPC(ch) )
     {
   bug( "Set_title: NPC.", 0 );
   return;
     }
     

     if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' )
     {
   buf[0] = ' ';
   strcpy( buf+1, title );
     }
     else
     {
   strcpy( buf, title );
     }
 
     free_string( ch->pcdata->title );
     ch->pcdata->title = str_dup( buf );
     return;
 }
 
 
 
 void do_title( CHAR_DATA *ch, char *argument )
 {
 	int i, limit;
	int count = 0;
     if ( IS_NPC(ch) )
   return;

     if ( IS_SET(ch->comm,COMM_NOTITLE) )
     {
   send_to_char( "Ok.\n\r",ch);
   return;
     }

     if ( argument[0] == '\0' )
     {
   send_to_char( "Change your title to what?\n\r", ch );
   return;
     }
 
 		limit = strlen(argument);
 		if(argument[limit - 1] == '{')
 		{// This one is fun, {{ escapes to { - count number of {'s
 			count = 0;
			for(i = limit - 1; i >= 0; i--)
 			{
 				if(argument[i] == '{')
 					count++;
 				else
 					break;// Found no {
 			}
 			if((count & 1) == 1)
 			{// Odd number, kill one
 				argument[limit - 1] = '\0';
 				limit--;// It's one shorter
 				if(limit <= 0)
 				{
 				 send_to_char("Change your title to what?\n\r", ch);
 				 return;
 				}
 			}
 		}
 		if(limit > 45)
 		{
 			count = 0;
	 		for(i = 0; i < 45; i++)
	 		{
	 			if(argument[i + count] == '{')
	 			{
	 				if(argument[i + count + 1] == '{')
	 				{
	 					count++;// It's displaying one of these two
	 				}
	 				else
	 				{
	 					count += 2;// It's skipping both of these
	 				}
	 			}
				else if(argument[i + count] == '\0')
					break;
				if(count > 90)
					break;
	 		}
	 	}
  if ( limit > 45 + count )
   argument[45 + count] = '\0';
 
     smash_tilde( argument );
     set_title( ch, argument );
     send_to_char( "Your title is: ", ch );
     send_to_char(argument, ch);
     send_to_char("\n\r", ch);
 } 
 
 
 void do_description( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
 
     if ( argument[0] != '\0' )
     {
   buf[0] = '\0';
   smash_tilde( argument );
 
   if (argument[0] == '-')
   {
       int len;
       bool found = FALSE;
  
       if (ch->description == NULL || ch->description[0] == '\0')
       {
     send_to_char("No lines left to remove.\n\r",ch);
     return;
       }
   
       strcpy(buf,ch->description);
  
       for (len = strlen(buf); len > 0; len--)
       {
     if (buf[len] == '\r')
     {
         if (!found)  /* back it up */
         {
       if (len > 0)
           len--;
       found = TRUE;
         }
         else /* found the second one */
         {
       buf[len + 1] = '\0';
       free_string(ch->description);
       ch->description = str_dup(buf);
       send_to_char( "Your description is:\n\r", ch );
       send_to_char( ch->description ? ch->description : 
           "(None).\n\r", ch );
       return;
         }
     }
       }
       buf[0] = '\0';
       free_string(ch->description);
       ch->description = str_dup(buf);
       send_to_char("Description cleared.\n\r",ch);
       return;
   }
   if ( argument[0] == '+' )
   {
       if ( ch->description != NULL )
     strcat( buf, ch->description );
       argument++;
       while ( isspace(*argument) )
     argument++;
   }
 
   if ( strlen(buf) + strlen(argument) >= MAX_STRING_LENGTH - 2 )
   {
       send_to_char( "Description too long.\n\r", ch );
       return;
   }
 
   strcat( buf, argument );
   strcat( buf, "\n\r" );
   free_string( ch->description );
   ch->description = str_dup( buf );
     }
 
     send_to_char( "Your description is:\n\r", ch );
     send_to_char( ch->description ? ch->description : "(None).\n\r", ch );
     return;
 }
 
 
void do_kr( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char tbuf[MAX_INPUT_LENGTH];

  if( IS_SET(ch->mhs,MHS_HIGHLANDER))
  {
     sprintf(buf,"$n says 'I have claimed the souls of %d victims, %d of them by my own hand.'\n\r",
	    ch->pcdata->highlander_data[ALL_KILLS],
	    ch->pcdata->highlander_data[REAL_KILLS]);
     act(buf,ch,NULL,NULL,TO_ROOM,FALSE);
    
     sprintf(buf,"You have claimed the souls of %d victims, %d of them by your own hand.\n\r",
	    ch->pcdata->highlander_data[ALL_KILLS],
	    ch->pcdata->highlander_data[REAL_KILLS]);
     send_to_char( buf,ch);
     return;
  }

  if( IS_NPC(ch) || (!is_clan(ch) 
		     && !IS_IMMORTAL(ch)
		     && !IS_SET(ch->mhs,MHS_GLADIATOR)) )
	return;

  sprintf( buf, "You have %d lower kills, %d equal kills and %d greater kills.\n\r"
		"The last player you killed was %s.\n\r"
		"The last player to kill you was %s.\n\r"
		"You have %d Single Victories, %d Single Kills in %d events,\n\r"
		"%d Team Victories and %d Team Kills in %d events.\n\r",
		ch->pcdata->killer_data[PC_LOWER_KILLS],
		ch->pcdata->killer_data[PC_EQUAL_KILLS],
		ch->pcdata->killer_data[PC_GREATER_KILLS],
                ch->pcdata->last_kill,
		ch->pcdata->last_killed_by,
		ch->pcdata->gladiator_data[GLADIATOR_VICTORIES],
		ch->pcdata->gladiator_data[GLADIATOR_KILLS],
		ch->pcdata->gladiator_data[GLADIATOR_PLAYS],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS]);
  send_to_char( buf, ch );
  if (ch->pcdata->steal_data[PC_STOLEN_ITEMS] > 0 ||
      ch->pcdata->steal_data[PC_STOLEN_GOLD] > 0 ) 
  {
     sprintf( buf,"You have stolen %ld items and %ld gold.\n\r",
	      ch->pcdata->steal_data[PC_STOLEN_ITEMS],  
	      ch->pcdata->steal_data[PC_STOLEN_GOLD]);
     send_to_char(buf,ch);
  }

  if (ch->pcdata->steal_data[PC_SLICES] > 0)
  {
     sprintf( buf,"You have sliced open  %ld items.\n\r",
	      ch->pcdata->steal_data[PC_SLICES]);
     send_to_char(buf,ch);
  }
  sprintf( buf, "To inform you, $n says 'I have %d lower kills," 
		"%d equal kills and %d greater kills.\n\r"
		"I last killed %s.  The last person to kill me was %s.\n\r"
		"In Single Arena I have %d victories having killed %d Gladiators in %d events.\n\r"
		"In Teams Arena I have %d victories having killed %d Gladiators in %d events.",
		ch->pcdata->killer_data[PC_LOWER_KILLS], 
                ch->pcdata->killer_data[PC_EQUAL_KILLS], 
                ch->pcdata->killer_data[PC_GREATER_KILLS],
		ch->pcdata->last_kill,ch->pcdata->last_killed_by,
		ch->pcdata->gladiator_data[GLADIATOR_VICTORIES],
		ch->pcdata->gladiator_data[GLADIATOR_KILLS],
		ch->pcdata->gladiator_data[GLADIATOR_PLAYS],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS],
		ch->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS]); 

  if (ch->pcdata->steal_data[PC_STOLEN_ITEMS] > 0 ||
      ch->pcdata->steal_data[PC_STOLEN_GOLD] > 0 ) 
  {
     sprintf( tbuf, "\n\rI have stolen %ld items and %ld gold.",  
		ch->pcdata->steal_data[PC_STOLEN_ITEMS],
		ch->pcdata->steal_data[PC_STOLEN_GOLD]); 
     strcat(buf,tbuf);
  }

  if (ch->pcdata->steal_data[PC_SLICES] > 0)
  {
     sprintf( tbuf, "\n\rI have sliced open %ld items.", 
		ch->pcdata->steal_data[PC_SLICES]); 
     strcat(buf,tbuf);
  }

  strcat(buf,"'\n\r");
  act( buf, ch, NULL, NULL, TO_ROOM,FALSE );

  return;

}


 void do_report( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_INPUT_LENGTH];
//     char wound[80];
//     char mental[80];
//     char moves[80];
     int percent = 0;

     if ( is_affected(ch, gsn_rage) )
     {
	 send_to_char("Report?  Report what?  You feel just FINE!\n\r",ch);
	 return;
     }
// OLD report with no numbers, new report is after it.
/*    if (ch->max_hit > 0)
    percent = ch->hit * 100 / ch->max_hit;
      else
    percent = -1;

      if (percent >= 100)
    strcpy(wound,"I am in excellent condition, ");
      else if (percent >= 90)
    strcpy(wound,"I have a few scratches, ");
      else if (percent >= 75)
    strcpy(wound,"I have some small wounds and bruises, ");
      else if (percent >= 50)
    strcpy(wound,"I have quite a few wounds, ");
      else if (percent >= 30)
    strcpy(wound,"I have some big nasty wounds and scratches, ");
      else if (percent >= 15)
    strcpy(wound,"I am pretty hurt, ");
      else if (percent >= 0)
    strcpy(wound,"I am in awful condition, ");
      else
    strcpy(wound,"I am bleeding to death, ");

      if (ch->max_mana > 0)
    percent = ch->mana * 100 / ch->max_mana;
      else
    percent = -1;

      if (percent >= 100)
    strcpy(mental,"am mentally fit ");
      else if (percent >= 90)
    strcpy(mental,"am a bit slow ");
      else if (percent >= 75)
    strcpy(mental,"have some mental lapses ");
      else if (percent >= 50)
    strcpy(mental,"am quite drained ");
      else if (percent >= 30)
    strcpy(mental,"feel dazed ");
      else if (percent >= 15)
    strcpy(mental,"am nearly spent ");
      else if (percent >= 0)
    strcpy(mental,"am brain dead ");
      else
    strcpy(mental,"am helpless ");

      if (ch->max_move > 0)
    percent = ch->move * 100 / ch->max_move;
      else
    percent = -1;

      if (percent >= 100)
    strcpy(moves,"and am full of energy.");
      else if (percent >= 90)
    strcpy(moves,"and am energetic.");
      else if (percent >= 75)
    strcpy(moves,"and am breathing hard.");
      else if (percent >= 50)
    strcpy(moves,"and my heart is pounding.");
      else if (percent >= 30)
    strcpy(moves,"and feel winded.");
      else if (percent >= 15)
    strcpy(moves,"and am cramping up.");
      else if (percent >= 0)
    strcpy(moves,"and am exhausted.");
      else
    strcpy(moves,"and am motionless.");
 
     sprintf( buf,
   "You say '%s%s%s %d xp.'\n\r",
   wound, mental, moves,
   ch->exp   );
 
     send_to_char( buf, ch );
 
     sprintf( buf, "$n says '%s%s%s %d xp.'",
   wound, mental, moves,
   ch->exp   );
 
     act( buf, ch, NULL, NULL, TO_ROOM, TRUE );
*/ 
   sprintf(buf, "You say 'I have %d/%d hp, %d/%d mana, %d/%d mv. %d xp.'", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp);
   send_to_char(buf,ch);

   sprintf(buf, "$n says 'I have %d/%d hp, %d/%d mana, %d/%d mv. %d xp.'", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp);
   act( buf, ch, NULL, NULL, TO_ROOM, TRUE );

     return;
 }
 
 
 
 void do_practice( CHAR_DATA *ch, char *argument )
 {
     bool fNew = FALSE, fSkills = FALSE, fSpells = FALSE;
     bool fOld = FALSE, fSkip = FALSE;
     char buf[MAX_STRING_LENGTH * 3];
     char buf2[MAX_STRING_LENGTH * 3];
     char arg[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     char *target_name;
     int sn;
 
     if ( IS_NPC(ch) )
   return;

/*
     if ( argument[0] == '-' )
     {
	argument = argument +1;
	if ( !str_cmp(argument,"skills") )
		fSkills = TRUE;
	else
	if ( !str_cmp(argument,"spells") )
		fSpells = TRUE;
	else
	if ( !str_cmp(argument,"new") )
		fNew = TRUE;
	else
	if ( !str_cmp(argument,"skip") )
		fSkip = TRUE;
	else
	if ( !str_cmp(argument,"old") )
		fOld = TRUE;
	else
		argument[0] = '\0';
     }
     */

     target_name = one_argument( argument, arg );
     one_argument( target_name, arg2 );

     if (!str_cmp(arg,"-old"))
	fOld = TRUE;
     else
     if (!str_cmp(arg,"-skip"))
	fSkip = TRUE;
     else
     if (!str_cmp(arg,"-new"))
	fNew = TRUE;
     else
     if (!str_cmp(arg,"-skills"))
	fSkills = TRUE;
     else
     if (!str_cmp(arg,"-spells"))
	fSpells = TRUE;

     if ( argument[0] == '\0' || fNew || fSkills || fSpells)
     {
        int col;
        buf2[0] = '\0';
 
        col    = 0;
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
           if ( skill_table[sn].name == NULL )
              break;
           if ( ch->level < skill_level(ch,sn)
               || ch->pcdata->learned[sn] < 1)
              continue;

     	   if ( fNew && ch->pcdata->learned[sn] >= 75 )
              continue;

	   if ( fSpells && skill_table[sn].spell_fun == spell_null )
              continue;

	   if ( fSkills && skill_table[sn].spell_fun != spell_null )
	      continue;

           sprintf( buf, "{%c%-18s{x %3d%%  " , 
              ch->pcdata->learned[sn] != ch->pcdata->last_learned[sn] ? 'R' : 'x',
              skill_table[sn].name,
              ch->pcdata->learned[sn] );
              strcat( buf2, buf );

           if ( ++col % 3 == 0 )
              strcat( buf2, "\n\r" );
           ch->pcdata->last_learned[sn] = ch->pcdata->learned[sn];
        }
 
        if ( col % 3 != 0 )
           strcat( buf2, "\n\r" );

        sprintf( buf, "You have %d practice sessions left.\n\r",
           ch->practice );
        strcat( buf2, buf );
        page_to_char(buf2, ch);
     }
     else
     {
        CHAR_DATA *mob;
        int adept, blah;
 
        if ( !IS_AWAKE(ch) )
        {
           send_to_char( "In your dreams, or what?\n\r", ch );
           return;
        }
 
        for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
        {
           if ( IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE) )
              break;
        }
 
        if ( mob == NULL )
        {
           send_to_char( "You can't do that here.\n\r", ch );
           return;
        }
 
        if ( ch->practice <= 0 )
        {
           send_to_char( "You have no practice sessions left.\n\r", ch );
           return;
        }

        if (arg2[0] != '\0')
	   strcpy(arg,arg2);

        if ( ( sn = find_spell( ch,arg ) ) < 0
               || ( !IS_NPC(ch)
               &&   (ch->level < skill_level(ch,sn)
               ||    ch->pcdata->learned[sn] < 1 
               ||    skill_table[sn].rating[ch->class] == 0)))
        {
           send_to_char( "You can't practice that.\n\r", ch );
           return;
        }

        if (ch->pcdata->learned[sn] < ch->pcdata->old_learned[sn]
            && !fOld && !fSkip)
        {
           sprintf(buf,"You had %s at %d prior to the Pfresh.\n\r",
	      skill_table[sn].name,ch->pcdata->old_learned[sn] ); 
	   send_to_char(buf,ch);
           send_to_char("If you would like to return to that %, use 'practice -old (name)'.\n\r", ch);
           send_to_char("If you would like to disregard your old %, use 'practice -skip (name)'.\n\r",ch);
           return;
        }

        adept = IS_NPC(ch) ? 100 : class_table[ch->class].skill_adept;


        if (fOld)
        {
           if (ch->pcdata->learned[sn] < ch->pcdata->old_learned[sn])
           {
              ch->practice--;
              ch->pcdata->learned[sn] = ch->pcdata->old_learned[sn]; 

              if ( ch->pcdata->learned[sn] < 100 )
              {
                 act( "You jump around yelling 'Pfresh my $T!'",
                    ch, NULL, skill_table[sn].name, TO_CHAR, TRUE );
                 act( "$n jumps around yelling 'Pfresh my $T!'",
                    ch, NULL, skill_table[sn].name, TO_ROOM, TRUE );
              }
              else
              {
                 act( "You head is bursting with all the knowledge of $T!",
                    ch, NULL, skill_table[sn].name, TO_CHAR, TRUE );
                 act( "$n grabs their head and screams 'Yikes my $T!'",
                    ch, NULL, skill_table[sn].name, TO_ROOM, TRUE);
              }
           }
           else
           {
      send_to_char("Your skill is already higher then your old skill.\n\r",ch);
           }
        }
        else
        {
           if ( ch->pcdata->learned[sn] >= adept )
           {
              sprintf( buf, "You are already learned at %s.\n\r",
                 skill_table[sn].name );
              send_to_char( buf, ch );
           }
	   else
	   {
              blah = abs(skill_table[sn].rating[ch->class]);
              ch->practice--;
	      if ( adept > 75 && ch->pcdata->learned[sn] > 75 )
		ch->pcdata->learned[sn]++;
	      else
              ch->pcdata->learned[sn] += 
                 int_app[get_curr_stat(ch,STAT_INT)].learn / blah ;

              if ( ch->pcdata->learned[sn] < adept )
              {
                 act( "You practice $T.",
                    ch, NULL, skill_table[sn].name, TO_CHAR, FALSE );
                 act( "$n practices $T.",
                    ch, NULL, skill_table[sn].name, TO_ROOM, FALSE );
              }
              else
              {
                 ch->pcdata->learned[sn] = adept;
                 act( "You are now learned at $T.",
                    ch, NULL, skill_table[sn].name, TO_CHAR, FALSE );
                 act( "$n is now learned at $T.",
                    ch, NULL, skill_table[sn].name, TO_ROOM, FALSE );
              }
	   }
        }
     }
     return;
 }
 
 
 
 /*
  * 'Wimpy' originally by Dionysos.
  */
 void do_wimpy( CHAR_DATA *ch, char *argument )
 {
     char buf[MAX_STRING_LENGTH];
     char arg[MAX_INPUT_LENGTH];
     int wimpy;
 
     one_argument( argument, arg );
 
     if ( arg[0] == '\0' )
   wimpy = 20;
     else
   wimpy = atoi( arg );
 
     if ( wimpy < 0 )
     {
   send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
   return;
     }
 
     if ( wimpy > 50 )
     {
   send_to_char( "Such cowardice ill becomes you.\n\r", ch );
   return;
     }
 
     ch->wimpy   = wimpy;
     sprintf( buf, "Wimpy set to %d percent.\n\r", wimpy );
     send_to_char( buf, ch );
     return;
 }
 
 
 
 void do_password( CHAR_DATA *ch, char *argument )
 {
     char arg1[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     char log_buf[MAX_INPUT_LENGTH];
     char *pArg;
     char *pwdnew;
     char *p;
     char cEnd;
 
     if ( IS_NPC(ch) )
   return;
 
     /*
      * Can't use one_argument here because it smashes case.
      * So we just steal all its code.  Bleagh.
      */
     pArg = arg1;
     while ( isspace(*argument) )
   argument++;
 
     cEnd = ' ';
     if ( *argument == '\'' || *argument == '"' )
   cEnd = *argument++;
 
     while ( *argument != '\0' )
     {
   if ( *argument == cEnd )
   {
       argument++;
       break;
   }
   *pArg++ = *argument++;
     }
     *pArg = '\0';
 
     pArg = arg2;
     while ( isspace(*argument) )
   argument++;
 
     cEnd = ' ';
     if ( *argument == '\'' || *argument == '"' )
   cEnd = *argument++;
 
     while ( *argument != '\0' )
     {
   if ( *argument == cEnd )
   {
       argument++;
       break;
   }
   *pArg++ = *argument++;
     }
     *pArg = '\0';
 
     sprintf( log_buf, "Log %s: password %s %s", ch->name, arg1, arg2 );
     log_string( log_buf ); 

     if ( arg1[0] == '\0' || arg2[0] == '\0' )
     {
   send_to_char( "Syntax: password <old> <new>.\n\r", ch );
   return;
     }

     if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
     {
   WAIT_STATE( ch, 40 );
   send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
   return;
     }
 
     if ( strlen(arg2) < 5 )
     {
   send_to_char(
       "New password must be at least five characters long.\n\r", ch );
   return;
     }
 
     /*
      * No tilde allowed because of player file format.
      */
     pwdnew = crypt( arg2, ch->name );
     for ( p = pwdnew; *p != '\0'; p++ )
     {
   if ( *p == '~' )
   {
       send_to_char(
     "New password not acceptable, try again.\n\r", ch );
       return;
   }
     }
 
     free_string( ch->pcdata->pwd );
     ch->pcdata->pwd = str_dup( pwdnew );
     save_char_obj( ch );
     send_to_char( "Ok.\n\r", ch );
     return;
 }
 
 
void do_ignore( CHAR_DATA *ch, char *argument )
 {
     char arg[MAX_INPUT_LENGTH];
     CHAR_DATA *ignore;

  one_argument(argument,arg);
  if( arg[0] == '\0' )
	{
	 send_to_char("Ignore reset.\n\r",ch);
            ch->ignoring = NULL;
            return;
          }

  if ( (ignore = get_char_world(ch, arg) ) == NULL)
  {
     send_to_char("They aren't here.\n\r",ch);
     return;
  }

            
  if ( IS_IMMORTAL(ignore) )
  {
     send_to_char("You can't ignore Immortals, dumbass.\n\r",ch);
     return;
  }

  if ( ignore != NULL && !IS_NPC(ignore) )
	{ 
	ch->ignoring = ignore;
	if(ch->ignoring == ch) 
	  { 
	    send_to_char("Ignore reset.\n\r",ch);
	    ch->ignoring = NULL;
	    return;
	  }
	 send_to_char( "Ignoring.\n\r", ch);
	}
  else
	{
	 send_to_char( "Invalid target.\n\r", ch);
	}
 }

void do_hd( CHAR_DATA *ch, char *argument )
{
     char bufbuf[MAX_STRING_LENGTH];
     bufbuf[0] = '\0';

     if (IS_NPC(ch))
	return;

    strcat(bufbuf,"Naturally, you can hit ");
    if ( GET_HITROLL(ch) < 0 ) strcat(bufbuf, "the broad side of a barn.\r\n");
    if ( GET_HITROLL(ch) >= 0 && GET_HITROLL(ch) <= 2) strcat(bufbuf, "things pointed out to you.\r\n");
    if ( GET_HITROLL(ch) >= 3 && GET_HITROLL(ch) <= 5) strcat(bufbuf, "better than a kender warrior.\r\n");
    if ( GET_HITROLL(ch) >= 6 && GET_HITROLL(ch) <= 8 ) strcat(bufbuf, "things alright, but you could do better.\r\n");
    if ( GET_HITROLL(ch) >= 9 && GET_HITROLL(ch) <= 10 ) strcat(bufbuf, "pretty good.  That night class in melee really helped you out.\r\n");
    if ( GET_HITROLL(ch) >= 11 && GET_HITROLL(ch) <= 17 ) strcat(bufbuf, "things fairly well.\r\n");
    if ( GET_HITROLL(ch) >= 18 ) strcat(bufbuf, "things really really well.  Good job!\r\n");

    strcat(bufbuf,"With your magical bonuses, you can hit ");
    if ( GET_SECOND_HITROLL(ch) < 0 ) strcat(bufbuf, "the broad side of a barn.\r\n");
    if ( GET_SECOND_HITROLL(ch) >= 0 && GET_SECOND_HITROLL(ch)<= 5) strcat(bufbuf, "things affected by continual light.\r\n");
    if ( GET_SECOND_HITROLL(ch) >= 6 && GET_SECOND_HITROLL(ch)<= 10 ) strcat(bufbuf, "things alright, but you could do better.\r\n");
    if ( GET_SECOND_HITROLL(ch) >= 11 && GET_SECOND_HITROLL(ch)<= 17 ) strcat(bufbuf, "a black cat in a dark cellar at midnight.\r\n");
    if ( GET_SECOND_HITROLL(ch) >= 18 ) strcat(bufbuf, "anything.  You can't miss.  Nice weapons skills.\r\n");

    strcat(bufbuf,"Naturally, when you hit things ");
    if ( GET_DAMROLL(ch) < 0 ) strcat(bufbuf, "your oppoents look at you funny.\r\n");
    if ( GET_DAMROLL(ch) >= 0 && GET_DAMROLL(ch)<= 5) strcat(bufbuf, "your target says \"Ow.  That hurt.\"\r\n");
    if ( GET_DAMROLL(ch) >= 6 && GET_DAMROLL(ch)<= 10 ) strcat(bufbuf, "your victim calls their HMO to see if this wound is covered.\r\n");
    if ( GET_DAMROLL(ch) >= 11 && GET_DAMROLL(ch)<= 17 ) strcat(bufbuf, " people fall down and go {YBOOM{x!\r\n");
    if ( GET_DAMROLL(ch) >= 18 ) strcat(bufbuf, "shit happens.\r\n");

    strcat(bufbuf,"After factoring in your bonuses, when you hit things ");
    if ( GET_SECOND_DAMROLL(ch) < 0 ) strcat(bufbuf, "the whole SLED laughs at you.\r\n");
    if ( GET_SECOND_DAMROLL(ch) >= 0 && GET_SECOND_DAMROLL(ch)<= 5) strcat(bufbuf, "people sit up and pay attention.\r\n");
    if ( GET_SECOND_DAMROLL(ch) >= 6 && GET_SECOND_DAMROLL(ch)<= 10 ) strcat(bufbuf, "blood is drawn and body parts fall off.\r\n");
    if ( GET_SECOND_DAMROLL(ch) >= 11 && GET_SECOND_DAMROLL(ch)<= 17 ) strcat(bufbuf, "you do some serious damage.\r\n");
    if ( GET_SECOND_DAMROLL(ch) >= 18 ) strcat(bufbuf, "nothing is left except a pale pink mist.\r\n");



     send_to_char( bufbuf, ch );

     sprintf( bufbuf, "\n\rHitroll: %d/%d  Damroll: %d/%d.\n\r",
             GET_HITROLL(ch),GET_SECOND_HITROLL(ch), 
             GET_DAMROLL(ch),GET_SECOND_DAMROLL(ch) );
     send_to_char(bufbuf, ch);
}

bool check_match( CHAR_DATA *ch, CHAR_DATA *victim)
{

  if (!IS_SET(ch->display, DISP_BRIEF_SCAN))
     return TRUE;

  if ( (is_clan(ch) && is_clan(victim)) ||
       (!is_clan(ch) && !is_clan(victim)) )
	 return TRUE;

  return FALSE;
}

void do_enemy( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;
   char arg1[MAX_INPUT_LENGTH];

/* Enemy code killed by NIGHTDAGGER on 04/25/2003 */
   send_to_char("Enemy lists don't exist anymore.\n\r",ch);
   return;

/*   if (ch->clan != clan_lookup("warlock")
       && ch->clan != clan_lookup("zealot")
       && ch->clan != clan_lookup("honor")
       && ch->clan != clan_lookup("posse"))
      return;*/

   one_argument(argument, arg1);

   if( ch->pcdata->rank == MAX_RANK && arg1[0] != '\0' )
   {
      if ( (victim = get_char_world(ch, arg1)) == NULL )
      {
         send_to_char("They aren't here.\n\r", ch);
         return;
      }

      if (IS_NPC(victim))
      {
         send_to_char("Don't be a fucknut.\n\r", ch);
         return;
      }

/*      if (ch->clan == clan_lookup("warlock"))
         if (IS_SET(victim->mhs,MHS_WARLOCK_ENEMY))
	    REMOVE_BIT(victim->mhs,MHS_WARLOCK_ENEMY);

      if (ch->clan == clan_lookup("zealot"))
         if (IS_SET(victim->mhs,MHS_ZEALOT_ENEMY))
	    REMOVE_BIT(victim->mhs,MHS_ZEALOT_ENEMY);

      if (ch->clan == clan_lookup("posse"))
         if (IS_SET(victim->mhs,MHS_POSSE_ENEMY))
	    REMOVE_BIT(victim->mhs,MHS_POSSE_ENEMY);

      if (ch->clan == clan_lookup("honor"))
         if (IS_SET(victim->mhs,MHS_HONOR_ENEMY))
	    REMOVE_BIT(victim->mhs,MHS_HONOR_ENEMY);
*/
      send_to_char("Enemy Removed.",ch);
   }
   else
   {
/*      if(ch->clan == clan_lookup("warlock"))
      {
         strcpy( buf, "WARLOCK ENEMIES:\n\r");
         for (d = descriptor_list; d != NULL; d = d->next)
         {
            if (d->character != NULL)
            {
               if (IS_SET(d->character->mhs, MHS_WARLOCK_ENEMY))
               {
                  strcat( buf, d->character->name);
                  strcat( buf, "\n\r");
	       }
            }
	 }
      }
      if(ch->clan == clan_lookup("posse"))
      {
         strcpy( buf, "POSSE ENEMIES:\n\r");
         for (d = descriptor_list; d != NULL; d = d->next)
         {
            if (d->character != NULL)
            {
               if (IS_SET(d->character->mhs, MHS_POSSE_ENEMY))
               {
                  strcat( buf, d->character->name);
                  strcat( buf, "\n\r");
	       }
            }
	 }
      }
      if(ch->clan == clan_lookup("zealot"))
      {
         strcpy( buf, "ZEALOT ENEMIES:\n\r");
         for (d = descriptor_list; d != NULL; d = d->next)
         {
            if (d->character != NULL)
            {
               if (IS_SET(d->character->mhs, MHS_ZEALOT_ENEMY))
               {
                  strcat( buf, d->character->name);
                  strcat( buf, "\n\r");
	       }
            }
	 }
      }
      if(ch->clan == clan_lookup("honor"))
      {
         strcpy( buf, "HONOR ENEMIES:\n\r");
         for (d = descriptor_list; d != NULL; d = d->next)
         {
            if (d->character != NULL)
            {
               if (IS_SET(d->character->mhs, MHS_HONOR_ENEMY))
               {
                  strcat( buf, d->character->name);
                  strcat( buf, "\n\r");
	       }
            }
	 }
      }
      send_to_char( buf, ch );*/
   }
   return;
}

void do_surname( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument_cs( argument, arg );
    
    arg[0] = UPPER(arg[0]);

    if ( arg[0] == '\0' )
    {
	send_to_char("Syntax: surname <name>\n\r",ch);
	return;
    }

    if ( !str_cmp(arg,"show") )
    {
	send_to_char("The 'Show' family is plenty big already.\n\r",ch);
	return;
    }

    if ( !str_cmp(arg,"brief") )
    {
	send_to_char("Your really want to do 'brief surname' right? \n\r",ch);
	return;
    }

    if ( IS_NPC(ch) )
    {
	send_to_char("Your last name is Mob.\n\r",ch);
	return;
    }

    if ( ch->pcdata->surname != NULL )
    {
	send_to_char("You already picked a surname.\n\r",ch);
	return;
    }

    if ( strlen(arg) > 12 )
    {
	send_to_char("Surnames cannot be more than 12 characters long.\n\r",ch);
	return;
    }
 
    /* Same rules as first names */  
    if ( !check_parse_surname( arg ) )
    {
	send_to_char("That names contains invalid characters.\n\r",ch);
	return;
    }

    /* Whew!  I think that's all the snaity checking we need */
    ch->pcdata->surname = str_dup(arg);
    sprintf(buf,"Your surname is '%s'.\n\r",ch->pcdata->surname);
    send_to_char(buf,ch);
    return;
}

void do_newbie(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;

    if (ch->clan == clan_lookup("newbie"))  
    {
       ch->clan = 0;
       send_to_char("You have turned off your newbie flag.",ch);
       return;
    }
    else
    {
     send_to_char("Your newbie flag is off and can not be turned back on.",ch); 
     return;
    }
    return;
}
